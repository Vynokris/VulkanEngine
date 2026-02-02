#include "Resources/Cubemap.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "Core/Engine.h"
#include "Core/GpuDataManager.h"
#include <stb_image.h>
#include <vulkan/vulkan.h>
#include <iostream>
using namespace Core;
using namespace Resources;

Cubemap::Cubemap(std::array<std::string, 6> filenames)
    : names(std::move(filenames))
{
    // Load the texture data.
    for (uint32_t i = 0; i < 6; i++)
    {
        stbi_set_flip_vertically_on_load_thread(false);
        pixels[i] = stbi_load(names[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels[i]) {
            LogError(LogType::Resources, "Unable to load texture " + names[i]);
            return;
        }
    }
    mipLevels = (uint32_t)std::floor(std::log2(std::max(width, height))) + 1;

    // Send the texture data to the GPU and delete CPU data.
    Application::Get()->GetGpuData()->CreateData(*this);
    for (uint32_t i = 0; i < 6; i++)
    {
        stbi_image_free(pixels[i]);
        pixels[i] = nullptr;
    }
}

Cubemap::Cubemap(Cubemap&& other) noexcept
    : UniqueID(std::move(other)), names(std::move(other.names)), pixels(other.pixels), width(other.width), height(other.height), channels(other.channels), mipLevels(other.mipLevels)
{}

Cubemap& Cubemap::operator=(Cubemap&& other) noexcept
{
    UniqueID::operator=(std::move(other));
    names     = std::move(other.names);
    pixels    = other.pixels;
    width     = other.width;
    height    = other.height;
    channels  = other.channels;
    mipLevels = other.mipLevels;
    other.names     = {};
    other.pixels    = {};
    other.width     = 0;
    other.height    = 0;
    other.channels  = 0;
    other.mipLevels = 0;
    return *this;
}

Cubemap::~Cubemap()
{
    Application::Get()->GetGpuData()->DestroyData(*this);
    for (uint32_t i = 0; i < 6; i++)
    {
        if (pixels[i])
        {
            stbi_image_free(pixels[i]);
            pixels[i] = nullptr;
        }
    }
}


template<> const GpuArray<Cubemap>& GpuDataManager::CreateArray()
{
    if (cubemapsArray.vkDescriptorSetLayout && cubemapsArray.vkDescriptorPool) return cubemapsArray;

    // Get the necessary vulkan resources.
    const VkDevice vkDevice = renderer->GetVkDevice();
     
    // Set the binding of cubemap textures.
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding         = 0;
    layoutBinding.descriptorCount = 1;
    layoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Create the descriptor set layout.
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings    = &layoutBinding;
    if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &cubemapsArray.vkDescriptorSetLayout) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create descriptor set layout.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_SET_LAYOUT_ERROR");
    }

    // Set the type and number of descriptors.
    VkDescriptorPoolSize poolSize{};
    poolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = Engine::MAX_CUBEMAPS;

    // Create the descriptor pool.
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = &poolSize;
    poolInfo.maxSets       = Engine::MAX_CUBEMAPS;
    if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &cubemapsArray.vkDescriptorPool) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create descriptor pool.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_POOL_ERROR");
    }

    return cubemapsArray;
}

template<> const GpuData<Cubemap>& GpuDataManager::CreateData(const Cubemap& resource)
{
    using namespace GraphicsUtils;

    if (resource.GetID() == UniqueID::unassigned) {
        LogError(LogType::Resources, "Can't create GPU data from unassigned resource.");
        throw std::runtime_error("RESOURCE_UNASSIGNED_ERROR");
    }
    if (cubemaps.find(resource.GetID()) != cubemaps.end()) return cubemaps.at(resource.GetID());
    
    GpuData<Cubemap>& data = cubemaps.emplace(std::make_pair(resource.GetID(), GpuData<Cubemap>())).first->second;
    data.vkImageFormat = VK_FORMAT_R8G8B8A8_SRGB;

    // Get necessary vulkan resources.
    const VkDevice         device         = renderer->GetVkDevice();
    const VkPhysicalDevice physicalDevice = renderer->GetVkPhysicalDevice();
    const VkCommandPool    commandPool    = renderer->GetVkCommandPool();
    const VkQueue          graphicsQueue  = renderer->GetVkGraphicsQueue();

    // Get texture data.
    const int      width     = resource.GetWidth();
    const int      height    = resource.GetHeight();
    const uint32_t mipLevels = resource.GetMipLevels();
    const VkDeviceSize layerSize = (VkDeviceSize)(width * height) * 4;
    const VkDeviceSize imageSize = layerSize * 6;

    // Create a transfer buffer to send the pixels to the GPU.
    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(device, physicalDevice, imageSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    // Copy the pixels to the transfer buffer.
    void* mapMem;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &mapMem);
    for (uint32_t i = 0; i < 6; i++)
        memcpy((char*)mapMem + i * layerSize, resource.GetPixels(i), (size_t)layerSize);
    vkUnmapMemory(device, stagingBufferMemory);

    // Create the Vulkan image.
    CreateImage(device, physicalDevice, width, height, mipLevels, VK_SAMPLE_COUNT_1_BIT, data.vkImageFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                data.vkImage, data.vkImageMemory, true);

    // Copy the transfer buffer to the vulkan image.
    TransitionImageLayout(device, commandPool, graphicsQueue, data.vkImage, data.vkImageFormat, mipLevels, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, true);
    CopyBufferToImage    (device, commandPool, graphicsQueue, stagingBuffer, data.vkImage, (uint32_t)width, (uint32_t)height, true);

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
    barrier.subresourceRange.layerCount     = 6;
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
        blit.srcSubresource.layerCount = 6;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 6;

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
    CreateImageView(device, data.vkImage, data.vkImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, data.vkImageView, true);

    // Allocate the descriptor set.
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = cubemapsArray.vkDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &cubemapsArray.vkDescriptorSetLayout;
    if (vkAllocateDescriptorSets(device, &allocInfo, &data.vkDescriptorSet) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to allocate descriptor sets.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_SET_ALLOCATION_ERROR");
    }

    // Populate the descriptor set.
    VkDescriptorImageInfo imageInfo{};
    const GpuData<Cubemap>* gpuData = GetData(resource);
    imageInfo.imageView   = gpuData ? gpuData->vkImageView : VK_NULL_HANDLE;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.sampler     = renderer->GetVkTextureSampler();
    
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet          = data.vkDescriptorSet;
    descriptorWrite.dstBinding      = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo      = &imageInfo;
    
    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    
    return data;
}
