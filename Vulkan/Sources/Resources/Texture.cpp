#include "Resources/Texture.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "Core/GpuDataManager.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <vulkan/vulkan.h>
#include <iostream>
using namespace Core;
using namespace Resources;

Texture::Texture(std::string filename, const bool& containsColorData)
    : name(std::move(filename)), containsColor(containsColorData)
{
    // Load the texture data.
    stbi_set_flip_vertically_on_load_thread(true);
    pixels = stbi_load(name.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        LogError(LogType::Resources, "Unable to load texture " + name);
        return;
    }
    mipLevels = (uint32_t)std::floor(std::log2(std::max(width, height))) + 1;

    // Send the texture data to the GPU and delete CPU data.
    Application::Get()->GetGpuData()->CreateData(*this);
    stbi_image_free(pixels);
    pixels = nullptr;
}

Texture::Texture(Texture&& other) noexcept
    : UniqueID(std::move(other)), name(std::move(other.name)), containsColor(other.containsColor), width(other.width), height(other.height), channels(other.channels), mipLevels(other.mipLevels), pixels(other.pixels)
{}

Texture& Texture::operator=(Texture&& other) noexcept
{
    UniqueID::operator=(std::move(other));
    name      = std::move(other.name);
    containsColor   = other.containsColor;
    width     = other.width;
    height    = other.height;
    channels  = other.channels;
    mipLevels = other.mipLevels;
    pixels    = other.pixels;
    other.name      = "";
    other.width     = 0;
    other.height    = 0;
    other.channels  = 0;
    other.mipLevels = 0;
    other.pixels    = nullptr;
    return *this;
}

Texture::~Texture()
{
    Application::Get()->GetGpuData()->DestroyData(*this);
    if (pixels)
    {
        stbi_image_free(pixels);
        pixels = nullptr;
    }
}


template<> const GpuData<Texture>& GpuDataManager::CreateData(const Texture& resource)
{
    using namespace GraphicsUtils;

    if (resource.GetID() == UniqueID::unassigned) {
        LogError(LogType::Resources, "Can't create GPU data from unassigned resource.");
        throw std::runtime_error("RESOURCE_UNASSIGNED_ERROR");
    }
    if (textures.find(resource.GetID()) != textures.end()) return textures.at(resource.GetID());
    
    GpuData<Texture>& data = textures.emplace(std::make_pair(resource.GetID(), GpuData<Texture>())).first->second;
    data.vkImageFormat = resource.ContainsColorData() ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

    // Get necessary vulkan resources.
    const VkDevice         device         = renderer->GetVkDevice();
    const VkPhysicalDevice physicalDevice = renderer->GetVkPhysicalDevice();
    const VkCommandPool    commandPool    = renderer->GetVkCommandPool();
    const VkQueue          graphicsQueue  = renderer->GetVkGraphicsQueue();

    // Get texture data.
    const int      width     = resource.GetWidth();
    const int      height    = resource.GetHeight();
    const uint32_t mipLevels = resource.GetMipLevels();
    const VkDeviceSize imageSize = (VkDeviceSize)(width * height) * 4;

    // Create a transfer buffer to send the pixels to the GPU.
    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(device, physicalDevice, imageSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    // Copy the pixels to the transfer buffer.
    void* mapMem;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &mapMem);
    memcpy(mapMem, resource.GetPixels(), (size_t)imageSize);
    vkUnmapMemory(device, stagingBufferMemory);

    // Create the Vulkan image.
    CreateImage(device, physicalDevice, width, height, mipLevels, VK_SAMPLE_COUNT_1_BIT, data.vkImageFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                data.vkImage, data.vkImageMemory);

    // Copy the transfer buffer to the vulkan image.
    TransitionImageLayout(device, commandPool, graphicsQueue, data.vkImage, data.vkImageFormat, mipLevels, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CopyBufferToImage    (device, commandPool, graphicsQueue, stagingBuffer, data.vkImage, (uint32_t)width, (uint32_t)height);

    // Check if image format supports linear blitting.
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, data.vkImageFormat, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        LogError(LogType::Resources, "Texture image format does not support linear blitting.");
        throw std::runtime_error("TEXTURE_BLITTING_ERROR");
    }

    // Begin commands and create a memory barrier used to create mipmaps.
    const VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);
    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image                           = data.vkImage;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.subresourceRange.levelCount     = 1;

    // Create texture mipmaps.
    int32_t mipWidth  = width;
    int32_t mipHeight = height;
    for (uint32_t i = 1; i < mipLevels; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            data.vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            data.vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr, 0, nullptr, 1, &barrier);

    EndSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer);

    // Cleanup allocated resources.
    vkDestroyBuffer(device, stagingBuffer,       nullptr);
    vkFreeMemory   (device, stagingBufferMemory, nullptr);

    // Create the texture image view.
    CreateImageView(device, data.vkImage, data.vkImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, data.vkImageView);
    
    return data;
}
