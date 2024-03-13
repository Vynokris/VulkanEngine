#include "Resources/Texture.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <vulkan/vulkan.h>
#include <iostream>
using namespace Core;
using namespace Resources;

Texture::Texture(std::string filename)
    : name(std::move(filename))
{
    vkImageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    
    // Load the texture data.
    stbi_set_flip_vertically_on_load_thread(true);
    pixels = stbi_load(name.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    const VkDeviceSize imageSize = width * height * 4;
    if (!pixels) {
        LogError(LogType::Resources, "Unable to load texture " + name);
        return;
    }
    mipLevels = (uint32_t)std::floor(std::log2(std::max(width, height))) + 1;

    // Get the renderer and Vulkan device.
    const Renderer*          renderer       = Application::Get()->GetRenderer();
    const VkDevice           device         = renderer->GetVkDevice();
    const VkPhysicalDevice   physicalDevice = renderer->GetVkPhysicalDevice();
    const VkCommandPool      commandPool    = renderer->GetVkCommandPool();
    const VkQueue            graphicsQueue  = renderer->GetVkGraphicsQueue();

    // Create a transfer buffer to send the pixels to the GPU.
    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    GraphicsUtils::CreateBuffer(device, physicalDevice, imageSize,
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                stagingBuffer, stagingBufferMemory);

    // Copy the pixels to the transfer buffer.
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, (size_t)imageSize);
    vkUnmapMemory(device, stagingBufferMemory);
    stbi_image_free(pixels);

    // Create the Vulkan image.
    GraphicsUtils::CreateImage(device, physicalDevice, width, height, mipLevels, VK_SAMPLE_COUNT_1_BIT, vkImageFormat, VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               vkImage, vkImageMemory);

    // Copy the transfer buffer to the vulkan image.
    GraphicsUtils::TransitionImageLayout(device, commandPool, graphicsQueue, vkImage, vkImageFormat, mipLevels, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    GraphicsUtils::CopyBufferToImage    (device, commandPool, graphicsQueue, stagingBuffer, vkImage, (uint32_t)width, (uint32_t)height);
    GenerateMipmaps();

    // Cleanup allocated structures.
    vkDestroyBuffer(device, stagingBuffer,       nullptr);
    vkFreeMemory   (device, stagingBufferMemory, nullptr);

    // Create the texture image view.
    GraphicsUtils::CreateImageView(device, vkImage, vkImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, vkImageView);
}

Texture::Texture(Texture&& other) noexcept
    : name(std::move(other.name)), width(other.width), height(other.height), channels(other.channels), mipLevels(other.mipLevels),
      vkImage(other.vkImage), vkImageMemory(other.vkImageMemory), vkImageView(other.vkImageView), vkImageFormat(other.vkImageFormat)
{
    other.vkImage       = nullptr;
    other.vkImageMemory = nullptr;
    other.vkImageView   = nullptr;
    other.vkImageFormat = {};
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    name          = std::move(other.name);
    width         = other.width;
    height        = other.height;
    channels      = other.channels;
    mipLevels     = other.mipLevels;
    vkImage       = other.vkImage;
    vkImageMemory = other.vkImageMemory;
    vkImageView   = other.vkImageView;
    vkImageFormat = other.vkImageFormat;
    other.name          = "";
    other.width         = 0;
    other.height        = 0;
    other.channels      = 0;
    other.mipLevels     = 0;
    other.vkImage       = nullptr;
    other.vkImageMemory = nullptr;
    other.vkImageView   = nullptr;
    other.vkImageFormat = {};
    return *this;
}

Texture::~Texture()
{
    const VkDevice vkDevice = Application::Get()->GetRenderer()->GetVkDevice();
    if (vkImage      ) vkDestroyImage    (vkDevice, vkImage,       nullptr);
    if (vkImageMemory) vkFreeMemory      (vkDevice, vkImageMemory, nullptr);
    if (vkImageView  ) vkDestroyImageView(vkDevice, vkImageView,   nullptr);
    vkImage       = nullptr;
    vkImageMemory = nullptr;
    vkImageView   = nullptr;
}

void Texture::GenerateMipmaps() const
{
    if (vkImageView) return;
    
    const Renderer*        renderer       = Application::Get()->GetRenderer();
    const VkDevice         device         = renderer->GetVkDevice();
    const VkPhysicalDevice physicalDevice = renderer->GetVkPhysicalDevice();
    const VkCommandPool    commandPool    = renderer->GetVkCommandPool();
    const VkQueue          graphicsQueue  = renderer->GetVkGraphicsQueue();

    // Check if image format supports linear blitting.
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, vkImageFormat, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        LogError(LogType::Resources, "Texture image format does not support linear blitting.");
        throw std::runtime_error("TEXTURE_BLITTING_ERROR");
    }

    const VkCommandBuffer commandBuffer = GraphicsUtils::BeginSingleTimeCommands(device, commandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image                           = vkImage;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.subresourceRange.levelCount     = 1;

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
            0, nullptr,
            0, nullptr,
            1, &barrier);

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
            vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

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
        0, nullptr,
        0, nullptr,
        1, &barrier);

    GraphicsUtils::EndSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer);
}
