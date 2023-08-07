#include "Core/VulkanUtils.h"
#include "Core/Application.h"
#include <vulkan/vulkan.h>
#include <chrono>
#include <set>
#include <limits>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <GLFW/glfw3.h>
using namespace Core;
using namespace VulkanUtils;

const unsigned int VulkanUtils::MAX_FRAMES_IN_FLIGHT = 3;

#ifdef NDEBUG
    const bool VulkanUtils::VALIDATION_LAYERS_ENABLED = false;
#else
    const bool VulkanUtils::VALIDATION_LAYERS_ENABLED = true;
#endif

const std::vector<const char*> VulkanUtils::VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> VulkanUtils::EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
};

SwapChainSupportDetails::SwapChainSupportDetails()
{
    capabilities = new VkSurfaceCapabilitiesKHR;
}
SwapChainSupportDetails::SwapChainSupportDetails(const SwapChainSupportDetails& other)
{
     capabilities = new VkSurfaceCapabilitiesKHR;
    *capabilities = *other.capabilities;
     formats      =  other.formats;
     presentModes =  other.presentModes;
}
SwapChainSupportDetails::~SwapChainSupportDetails()
{
    delete capabilities;
}

QueueFamilyIndices VulkanUtils::FindQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
    QueueFamilyIndices queueFamilyIndices;

    // Get an array of all the queue families on the given GPU.
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    // Get all queue family indices.
    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        // Find the graphics family.
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
            queueFamilyIndices.graphicsFamily = i;

        // Find the present family.
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            queueFamilyIndices.presentFamily = i;
        }

        // Exit if the queue family indices have all been set.
        if (queueFamilyIndices.IsComplete())
            break;
        ++i;
    }
    return queueFamilyIndices;
}

uint32_t VulkanUtils::FindMemoryType(const VkPhysicalDevice& device, const uint32_t& typeFilter, const VkMemoryPropertyFlags& properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    LogError(LogType::Vulkan, "Failed to find suitable memory type.");
    throw std::runtime_error("VULKAN_FIND_MEMORY_TYPE_ERROR");
}

VkFormat VulkanUtils::FindSupportedFormat(const VkPhysicalDevice& device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (const VkFormat& format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device, format, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR  && (props.linearTilingFeatures & features) == features)
            return format;
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return format;
    }

    LogError(LogType::Vulkan, "Failed to find supported format.");
    throw std::runtime_error("VULKAN_NO_SUPPORTED_FORMAT_ERROR");
}

VkSampleCountFlagBits VulkanUtils::GetMaxUsableSampleCount(const VkPhysicalDevice& device)
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);

    const VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT)  { return VK_SAMPLE_COUNT_8_BIT;  }
    if (counts & VK_SAMPLE_COUNT_4_BIT)  { return VK_SAMPLE_COUNT_4_BIT;  }
    if (counts & VK_SAMPLE_COUNT_2_BIT)  { return VK_SAMPLE_COUNT_2_BIT;  }

    return VK_SAMPLE_COUNT_1_BIT;
}

SwapChainSupportDetails VulkanUtils::QuerySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
    SwapChainSupportDetails details;

    // Get surface capabilities.
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, details.capabilities);

    // Get supported surface formats.
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    // Get supported presentation modes.
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR VulkanUtils::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    // Try to find the best surface format.
    for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
    {
        if (availableFormat.format     == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    // Return the first format as a last resort.
    return availableFormats[0];
}

VkPresentModeKHR VulkanUtils::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    // Try to find the best present mode (for triple buffering).
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    // Use FIFO present mode by default.
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanUtils::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    // If the current extent is already set, return it.
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    
    else
    {
        // Find the size of the window framebuffer.
        int width, height;
        glfwGetFramebufferSize(Application::Get()->GetWindow()->GetGlfwWindow(), &width, &height);
        VkExtent2D extent = { (uint32_t)width, (uint32_t)height };

        // Make sure the extent is between the minimum and maximum extents.
        extent.width  = std::clamp(extent.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return extent;
    }
}

bool VulkanUtils::CheckDeviceExtensionSupport(const VkPhysicalDevice& device)
{
    // Get the number of extensions on the given GPU.
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    // Get an array of all the extensions on the given GPU.
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    // Create an array of unique required extension names.
    std::set<std::string> requiredExtensions(EXTENSIONS.begin(), EXTENSIONS.end());

    // Make sure all required extensions are available.
    for (const VkExtensionProperties& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    return requiredExtensions.empty();
}

bool VulkanUtils::IsDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
    
    return FindQueueFamilies(device, surface).IsComplete()
        && CheckDeviceExtensionSupport(device)
        && QuerySwapChainSupport(device, surface).IsAdequate()
        && supportedFeatures.samplerAnisotropy;
}

// TODO: Put this method somewhere it belongs.
static std::vector<char> ReadBinFile(const std::string& filename)
{
    // Open the binary file and place the cursor at its end.
    std::ifstream f(filename, std::ios::ate | std::ios::binary);
    if (!f.is_open()) {
        LogError(LogType::FileIO, "Failed to open file: " + filename);
        throw std::runtime_error("FILE_IO_ERROR");
    }

    // Get the file size and allocate a buffer with it.
    const size_t fSize = (size_t)f.tellg();
    std::vector<char> buffer(fSize);

    // Move the cursor back to the start of the file and read it.
    f.seekg(0);
    f.read(buffer.data(), (std::streamsize)fSize);
    f.close();
    return buffer;
}

VkCommandBuffer VulkanUtils::BeginSingleTimeCommands(const VkDevice& device, const VkCommandPool& commandPool)
{
    // Create a new command buffer.
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool        = commandPool;
    allocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    // Start recording the new command buffer.
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void VulkanUtils::EndSingleTimeCommands(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, const VkCommandBuffer& commandBuffer)
{
    // Execute the command buffer and wait until it is done.
    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void VulkanUtils::CreateShaderModule(const VkDevice& device, const char* filename, VkShaderModule& shaderModule)
{
    // Read the shader code.
    const std::vector<char> shaderCode = ReadBinFile(filename);
    
    // Set shader module creation information.
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(shaderCode.data());

    // Create and return the shader module.
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create shader module.");
        throw std::runtime_error("VULKAN_SHADER_MODULE_ERROR");
    }
}

void VulkanUtils::CreateBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = size;
    bufferInfo.usage       = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create the buffer.
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create buffer.");
        throw std::runtime_error("VULKAN_BUFFER_CREATION_ERROR");
    }

    // Find the buffer's memory requirements.
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    // Find which memory type is most suitable for the buffer.
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    // Allocate the buffer's memory.
    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to allocate buffer memory.");
        throw std::runtime_error("VULKAN_BUFFER_ALLOCATION_ERROR");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void VulkanUtils::CopyBuffer(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, const VkDeviceSize& size)
{
    const VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);
    
    // Issue the command to copy the source buffer to the destination one.
    VkBufferCopy copyRegion{};
    copyRegion.size      = size;
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    EndSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer);
}

void VulkanUtils::CreateImage(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const uint32_t& width, const uint32_t& height, const uint32_t& mipLevels, const VkSampleCountFlagBits& numSamples, const VkFormat& format, const VkImageTiling& tiling, const VkImageUsageFlags& usage, const VkMemoryPropertyFlags& properties, VkImage& image, VkDeviceMemory& imageMemory)
{    
    // Create a vulkan image.
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = (uint32_t)width;
    imageInfo.extent.height = (uint32_t)height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = mipLevels;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = format;
    imageInfo.tiling        = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = usage;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples       = numSamples;
    imageInfo.flags         = 0; // Optional.
    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create texture image.");
        throw std::runtime_error("VULKAN_TEXTURE_IMAGE_ERROR");
    }

    // Allocate GPU memory for the image.
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);
    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to allocate texture image memory.");
        throw std::runtime_error("VULKAN_TEXTURE_IMAGE_MEMORY_ERROR");
    }
    vkBindImageMemory(device, image, imageMemory, 0);
}

void VulkanUtils::CreateImageView(const VkDevice& device, const VkImage& image, const VkFormat& format, const VkImageAspectFlags& aspectFlags, const uint32_t& mipLevels, VkImageView& imageView)
{
    // Set creation information for the image view.
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image    = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format   = format;

    // Set color channel swizzling.
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // Choose how the image is used and set the mipmap and layer counts.
    viewInfo.subresourceRange.aspectMask     = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    // Create the image view.
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create image view.");
        throw std::runtime_error("VULKAN_IMAGE_VIEW_ERROR");
    }
}

void VulkanUtils::TransitionImageLayout(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, const VkImage& image, const VkFormat& format, const uint32_t& mipLevels, const VkImageLayout& oldLayout, const VkImageLayout& newLayout)
{
    const VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = oldLayout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        LogError(LogType::Vulkan, "Unsupported layout transition.");
        throw std::runtime_error("VULKAN_UNSUPPORTED_LAYOUT_TRANSITION_ERROR");
    }
    
    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage,
        destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
    
    EndSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer);
}

void VulkanUtils::CopyBufferToImage(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, const VkBuffer& buffer, const VkImage& image, const uint32_t& width, const uint32_t& height)
{
    const VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);

    VkBufferImageCopy region{};
    region.bufferOffset                    = 0;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset                     = { 0, 0, 0 };
    region.imageExtent                     = { width, height, 1 };
    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
    
    EndSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer);
}