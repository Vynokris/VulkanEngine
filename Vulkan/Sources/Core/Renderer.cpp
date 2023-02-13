#include "Core/Renderer.h"
#include "Core/Application.h"
#include "Maths/Vertex.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <set>
#include <array>
#include <limits>
#include <algorithm>
#include <fstream>
#include <iostream>
using namespace Core;
using namespace VulkanUtils;

// TODO: Temporary.
const std::vector<Maths::TestVertex> vertices = {
    {{ -.5f, -.5f, .0f }, { 1.f, 0.f, 0.f }, { 0.f, 0.f }},
    {{  .5f, -.5f, .0f }, { 0.f, 1.f, 0.f }, { 1.f, 0.f }},
    {{  .5f,  .5f, .0f }, { 0.f, 0.f, 1.f }, { 1.f, 1.f }},
    {{ -.5f,  .5f, .0f }, { 1.f, 1.f, 1.f }, { 0.f, 1.f }},

    {{ -.5f, -.5f, -.5f }, { 1.f, 0.f, 0.f }, { 0.f, 0.f }},
    {{  .5f, -.5f, -.5f }, { 0.f, 1.f, 0.f }, { 1.f, 0.f }},
    {{  .5f,  .5f, -.5f }, { 0.f, 0.f, 1.f }, { 1.f, 1.f }},
    {{ -.5f,  .5f, -.5f }, { 1.f, 1.f, 1.f }, { 0.f, 1.f }}
};
const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};
static VkVertexInputBindingDescription GetBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding   = 0;
    bindingDescription.stride    = sizeof(Maths::TestVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}
static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    
    attributeDescriptions[0].binding  = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset   = offsetof(Maths::TestVertex, pos);
    
    attributeDescriptions[1].binding  = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset   = offsetof(Maths::TestVertex, color);

    attributeDescriptions[2].binding  = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset   = offsetof(Maths::TestVertex, texCoord);

    return attributeDescriptions;
}
// End temporary.


#pragma region VulkanUtils
namespace VulkanUtils
{
    // Use validation layers only in debug mode.
    #ifdef NDEBUG
        constexpr bool VALIDATION_LAYERS_ENABLED = false;
    #else
        constexpr bool VALIDATION_LAYERS_ENABLED = true;
    #endif

    // Define which validation layers should be used.
    const std::vector VALIDATION_LAYERS = {
        "VK_LAYER_KHRONOS_validation"
    };

    // Define which extensions should be used.
    const std::vector EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
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

    std::cout << "ERROR (Vulkan): Failed to find suitable memory type." << std::endl;
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

    std::cout << "ERROR (Vulkan): Failed to find supported format." << std::endl;
    throw std::runtime_error("VULKAN_NO_SUPPORTED_FORMAT_ERROR");
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
        std::cout << "ERROR (File IO): Failed to open file: " << filename << std::endl;
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
        std::cout << "ERROR (Vulkan): Failed to create shader module." << std::endl;
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
        std::cout << "ERROR (Vulkan): Failed to create buffer." << std::endl;
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
        std::cout << "ERROR (Vulkan): Failed to allocate buffer memory." << std::endl;
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

void VulkanUtils::CreateImage(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const uint32_t& width, const uint32_t& height, const VkFormat& format, const VkImageTiling& tiling, const VkImageUsageFlags& usage, const VkMemoryPropertyFlags& properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    // Create a vulkan image.
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = (uint32_t)width;
    imageInfo.extent.height = (uint32_t)height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = format;
    imageInfo.tiling        = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = usage;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags         = 0; // Optional.
    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to create texture image." << std::endl;
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
        std::cout << "ERROR (Vulkan): Failed to allocate texture image memory." << std::endl;
        throw std::runtime_error("VULKAN_TEXTURE_IMAGE_MEMORY_ERROR");
    }
    vkBindImageMemory(device, image, imageMemory, 0);
}

void VulkanUtils::CreateImageView(const VkDevice& device, const VkImage& image, const VkFormat& format, const VkImageAspectFlags& aspectFlags, VkImageView& imageView)
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
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    // Create the image view.
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to create image view." << std::endl;
        throw std::runtime_error("VULKAN_IMAGE_VIEW_ERROR");
    }
}

void VulkanUtils::TransitionImageLayout(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, const VkImage& image, const VkFormat& format, const VkImageLayout& oldLayout, const VkImageLayout& newLayout)
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
    barrier.subresourceRange.levelCount     = 1;
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
        std::cout << "ERROR (Vulkan): Unsupported layout transition." << std::endl;
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
#pragma endregion


Renderer::Renderer(const char* appName, const char* engineName)
{
    app                    = Application::Get();
    vkPhysicalDevice       = VK_NULL_HANDLE;
    vkDepthImageFormat     = VK_FORMAT_UNDEFINED;
    vkSwapChainImageFormat = VK_FORMAT_UNDEFINED;

    CheckValidationLayers();
    CreateVkInstance(appName, engineName);
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();
    vkDepthImageFormat = FindSupportedFormat(vkPhysicalDevice,
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
    CreateRenderPass();
    CreateDescriptorSetLayout();
    CreateGraphicsPipeline();
    CreateDepthResources();
    CreateFramebuffers();
    CreateCommandPool();
    CreateTextureSampler();
    texture = new Resources::Texture("Resources/Textures/WholesomeFish.png", this);
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
    CreateCommandBuffers();
    CreateSyncObjects();
}

Renderer::~Renderer()
{
    vkDeviceWaitIdle(vkDevice);

    delete texture;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vkDevice, vkRenderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(vkDevice, vkImageAvailableSemaphores[i], nullptr);
        vkDestroyFence    (vkDevice, vkInFlightFences[i],           nullptr);
        vkDestroyBuffer   (vkDevice, vkUniformBuffers[i],           nullptr);
        vkFreeMemory      (vkDevice, vkUniformBuffersMemory[i],     nullptr);
    }
    DestroySwapChain();
    vkDestroySampler            (vkDevice,   vkTextureSampler,      nullptr);
    vkDestroyDescriptorPool     (vkDevice,   vkDescriptorPool,      nullptr);
    vkDestroyBuffer             (vkDevice,   vkIndexBuffer,         nullptr);
    vkFreeMemory                (vkDevice,   vkIndexBufferMemory,   nullptr);
    vkDestroyBuffer             (vkDevice,   vkVertexBuffer,        nullptr);
    vkFreeMemory                (vkDevice,   vkVertexBufferMemory,  nullptr);
    vkDestroyCommandPool        (vkDevice,   vkCommandPool,         nullptr);
    vkDestroyPipeline           (vkDevice,   vkGraphicsPipeline,    nullptr);
    vkDestroyPipelineLayout     (vkDevice,   vkPipelineLayout,      nullptr);
    vkDestroyDescriptorSetLayout(vkDevice,   vkDescriptorSetLayout, nullptr);
    vkDestroyRenderPass         (vkDevice,   vkRenderPass,          nullptr);
    vkDestroyDevice             (vkDevice,                          nullptr);
    vkDestroySurfaceKHR         (vkInstance, vkSurface,             nullptr);
    vkDestroyInstance           (vkInstance,                        nullptr);
}

void Renderer::BeginRender()
{
    NewFrame();
    BeginRecordCmdBuf();
}

void Renderer::DrawFrame() const
{
    // Issue the command to draw the triangle.
    UpdateUniformBuffer();
    vkCmdBindDescriptorSets(vkCommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineLayout, 0, 1, &vkDescriptorSets[currentFrame], 0, nullptr);
    vkCmdDrawIndexed(vkCommandBuffers[currentFrame], (uint32_t)indices.size(), 1, 0, 0, 0);
}

void Renderer::EndRender()
{
    EndRecordCmdBuf();
    PresentFrame();
}


#pragma region Setup
void Renderer::CheckValidationLayers() const
{
    // Get the number of vulkan layers on the machine.
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    // Get all the vulkan layer names.
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // Make sure all required validation layers are installed on the machine.
    bool validationLayersError = false;
    for (const char* layerName : VALIDATION_LAYERS)
    {
        bool layerFound = false;

        for (const VkLayerProperties& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            validationLayersError = true;
            break;
        }
    }

    // Throw and error if a validation layer hasn't been found.
    if (VALIDATION_LAYERS_ENABLED && validationLayersError) {
        std::cout << "ERROR (Vulkan): Some of the requested validation layers are not available." << std::endl;
        throw std::runtime_error("VK_VALIDATION_LAYERS_ERROR");
    }
}

void Renderer::CreateVkInstance(const char* appName, const char* engineName)
{
    // Set application information.
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = appName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = engineName;
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_3;

    // Set vulkan instance creation information.
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Set validation layer information.
    if (VALIDATION_LAYERS_ENABLED) {
        createInfo.enabledLayerCount   = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    // Set glfw extension information.
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    createInfo.enabledExtensionCount   = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    // Create the vulkan instance.
    if (vkCreateInstance(&createInfo, nullptr, &vkInstance) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Unable to create a Vulkan instance." << std::endl;
        throw std::runtime_error("VULKAN_INIT_ERROR");
    }
}

void Renderer::CreateSurface()
{
    if (glfwCreateWindowSurface(vkInstance, app->GetWindow()->GetGlfwWindow(), nullptr, &vkSurface) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Unable to create a  window surface." << std::endl;
        throw std::runtime_error("VULKAN_WINDOW_SURFACE_ERROR");
    }
}

void Renderer::PickPhysicalDevice()
{
    // Get the number of GPUs with Vulkan support.
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);

    // Make sure at least one GPU supports Vulkan.
    if (deviceCount == 0) {
        std::cout << "ERROR (Vulkan): Couldn't find a GPU with Vulkan support." << std::endl;
        throw std::runtime_error("GPU_NO_VULKAN_SUPPORT_ERROR");
    }

    // Get an array of all GPUs with Vulkan support.
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

    // Try to find a suitable GPU to run Vulkan on.
    for (const auto& device : devices) {
        if (IsDeviceSuitable(device, vkSurface)) {
            vkPhysicalDevice = device;
            break;
        }
    }

    // Make sure a suitable GPU was found.
    if (vkPhysicalDevice == VK_NULL_HANDLE) {
        std::cout << "ERROR (Vulkan): Couldn't find a GPU with Vulkan support." << std::endl;
        throw std::runtime_error("GPU_NO_VULKAN_SUPPORT_ERROR");
    }
}

void Renderer::CreateLogicalDevice()
{
    const QueueFamilyIndices indices = FindQueueFamilies(vkPhysicalDevice, vkSurface);

    // Set creation information for all required queues.
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    for (const uint32_t& queueFamily : uniqueQueueFamilies)
    {
        const float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Specify the devices features to be used.
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    // Set the logical device creation information.
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount    = (uint32_t)queueCreateInfos.size();
    deviceCreateInfo.pQueueCreateInfos       = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures        = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(EXTENSIONS.size());
    deviceCreateInfo.ppEnabledExtensionNames = EXTENSIONS.data();
    if (VALIDATION_LAYERS_ENABLED) {
        deviceCreateInfo.enabledLayerCount   = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        deviceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    } else {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    // Create the logical device.
    if (vkCreateDevice(vkPhysicalDevice, &deviceCreateInfo, nullptr, &vkDevice) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to create a logical device." << std::endl;
        throw std::runtime_error("VULKAN_LOGICAL_DEVICE_ERROR");
    }

    // Get the graphics and present queue handles.
    vkGetDeviceQueue(vkDevice, indices.graphicsFamily.value(), 0, &vkGraphicsQueue);
    vkGetDeviceQueue(vkDevice, indices.presentFamily .value(), 0, &vkPresentQueue );
}

void Renderer::CreateSwapChain()
{
    const SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(vkPhysicalDevice, vkSurface);

    // Get the surface format, presentation mode and extent of the swap chain.
    const VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat( swapChainSupport.formats);
    const VkPresentModeKHR   presentMode   = ChooseSwapPresentMode  ( swapChainSupport.presentModes);
    const VkExtent2D         extent        = ChooseSwapExtent       (*swapChainSupport.capabilities);

    // Choose the number of images in the swap chain.
    uint32_t imageCount = swapChainSupport.capabilities->minImageCount + 1;
    if (swapChainSupport.capabilities->maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities->maxImageCount)
    {
        imageCount = swapChainSupport.capabilities->maxImageCount;
    }

    // Set creation information for the swap chain.
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = vkSurface;
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.presentMode      = presentMode;
    createInfo.oldSwapchain     = VK_NULL_HANDLE;

    // Set the used queue families.
    const QueueFamilyIndices indices = FindQueueFamilies(vkPhysicalDevice, vkSurface);
    const uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    
    if (indices.graphicsFamily != indices.presentFamily)
    {
        // If the graphics and present family are different, share the swapchain between them.
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    }
    else
    {
        // If they are the same, give total ownership of the swapchain to the queue.
        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;       // Optional.
        createInfo.pQueueFamilyIndices   = nullptr; // Optional.
    }

    // Set the transform to apply to swapchain images.
    createInfo.preTransform = swapChainSupport.capabilities->currentTransform;

    // Choose whether the alpha channel should be used for blending with other windows in the window system or ignored.
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    // Don't render pixels that are obscured by other windows.
    createInfo.clipped = VK_TRUE;

    // Create the swap chain.
    if (vkCreateSwapchainKHR(vkDevice, &createInfo, nullptr, &vkSwapChain) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to create a swap chain." << std::endl;
        throw std::runtime_error("VULKAN_SWAP_CHAIN_ERROR");
    }

    // Get the swap chain images.
    vkGetSwapchainImagesKHR(vkDevice, vkSwapChain, &imageCount, nullptr);
    vkSwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vkDevice, vkSwapChain, &imageCount, vkSwapChainImages.data());

    // Get the swap chain format.
    vkSwapChainImageFormat = surfaceFormat.format;
    vkSwapChainWidth       = extent.width;
    vkSwapChainHeight      = extent.height;
}

void Renderer::CreateImageViews()
{
    vkSwapChainImageViews.resize(vkSwapChainImages.size());
    for (size_t i = 0; i < vkSwapChainImages.size(); i++)
        CreateImageView(vkDevice, vkSwapChainImages[i], vkSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, vkSwapChainImageViews[i]);
}

void Renderer::CreateRenderPass()
{
    // Create a color buffer attachment.
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = vkSwapChainImageFormat;
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // No stencil is used so we don't care.
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Create the color attachment (retrieved from the shader's layout(location = 0) out vec4).
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Create a depth buffer attachment.
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format         = vkDepthImageFormat;
    depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Create the depth attachment (retrieved from the shader's layout(location = 1) out vec4).
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Create a single sub-pass.
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // Create a sub-pass dependency.
    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT          | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // Set the render pass creation information.
    const std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = (uint32_t)attachments.size();
    renderPassInfo.pAttachments    = attachments.data();
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependency;

    // Create the render pass.
    if (vkCreateRenderPass(vkDevice, &renderPassInfo, nullptr, &vkRenderPass) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to create render pass." << std::endl;
        throw std::runtime_error("VULKAN_RENDER_PASS_ERROR");
    }
}

void Renderer::CreateDescriptorSetLayout()
{
    // Set the binding of the uniform buffer object.
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding            = 0;
    uboLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount    = 1;
    uboLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional.

    // Set the binding for the texture sampler.
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding            = 1;
    samplerLayoutBinding.descriptorCount    = 1;
    samplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Create the descriptor set layout.
    const std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = (uint32_t)bindings.size();
    layoutInfo.pBindings    = bindings.data();
    if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &vkDescriptorSetLayout) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to create descriptor set layout." << std::endl;
        throw std::runtime_error("VULKAN_DESCRIPTOR_SET_LAYOUT_ERROR");
    }
}

void Renderer::CreateGraphicsPipeline()
{
    // Load vulkan fragment and vertex shaders.
    VkShaderModule vertShaderModule{};
    VkShaderModule fragShaderModule{};
    CreateShaderModule(vkDevice, "Shaders/VulkanVert.spv", vertShaderModule);
    CreateShaderModule(vkDevice, "Shaders/VulkanFrag.spv", fragShaderModule);

    // Set the vertex shader's pipeline stage and entry point.
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName  = "main";

    // Set the fragment shader's pipeline stage and entry point.
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    // Create an array of both pipeline stage info structures.
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // Setup vertex bindings and attributes.
    auto bindingDescription    = GetBindingDescription();
    auto attributeDescriptions = GetAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size();
    vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

    // Specify the kind of geometry to be drawn.
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Specify the region of the framebuffer to render to.
    VkViewport viewport{};
    viewport.x        = 0.f;
    viewport.y        = 0.f;
    viewport.width    = (float)vkSwapChainWidth;
    viewport.height   = (float)vkSwapChainHeight;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    // Specify the scissor rectangle (pixels outside it will be discarded).
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { vkSwapChainWidth, vkSwapChainHeight };

    // Create the dynamic states for viewport and scissor.
    std::vector dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
    dynamicState.pDynamicStates    = dynamicStates.data();
    
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    // Set rasterizer parameters.
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL; // Use VK_POLYGON_MODE_LINE to draw wireframe.
    rasterizer.lineWidth               = 1.f;
    rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
    // rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.f; // Optional.
    rasterizer.depthBiasClamp          = 0.f; // Optional.
    rasterizer.depthBiasSlopeFactor    = 0.f; // Optional.

    // Set multisampling parameters (disabled).
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = VK_FALSE;
    multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading      = 1.f;      // Optional.
    multisampling.pSampleMask           = nullptr;  // Optional.
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional.
    multisampling.alphaToOneEnable      = VK_FALSE; // Optional.

    // Depth and stencil buffer parameters.
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable       = VK_TRUE;
    depthStencil.depthWriteEnable      = VK_TRUE;
    depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds        = 0.f; // Optional.
    depthStencil.maxDepthBounds        = 1.f; // Optional.
    depthStencil.stencilTestEnable     = VK_FALSE;
    depthStencil.front                 = {}; // Optional.
    depthStencil.back                  = {}; // Optional.

    // Set color blending parameters for current framebuffer (alpha blending enabled).
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable         = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

    // Set color blending parameters for all framebuffers.
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable     = VK_FALSE;
    colorBlending.logicOp           = VK_LOGIC_OP_COPY; // Optional.
    colorBlending.attachmentCount   = 1;
    colorBlending.pAttachments      = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.f; // Optional.
    colorBlending.blendConstants[1] = 0.f; // Optional.
    colorBlending.blendConstants[2] = 0.f; // Optional.
    colorBlending.blendConstants[3] = 0.f; // Optional.

    // Set the pipeline layout creation information.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = 1;
    pipelineLayoutInfo.pSetLayouts            = &vkDescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;       // Optional.
    pipelineLayoutInfo.pPushConstantRanges    = nullptr; // Optional.

    // Create the pipeline layout.
    if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &vkPipelineLayout) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to create pipeline layout." << std::endl;
        throw std::runtime_error("VULKAN_PIPELINE_LAYOUT_ERROR");
    }

    // Set the graphics pipeline creation information.
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = 2;
    pipelineInfo.pStages             = shaderStages;
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = &depthStencil;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.layout              = vkPipelineLayout;
    pipelineInfo.renderPass          = vkRenderPass;
    pipelineInfo.subpass             = 0;
    pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE; // Optional.
    pipelineInfo.basePipelineIndex   = -1;             // Optional.

    // Create the graphics pipeline.
    if (vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkGraphicsPipeline) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to create graphics pipeline." << std::endl;
        throw std::runtime_error("VULKAN_GRAPHICS_PIPELINE_ERROR");
    }
    
    // Destroy both shader modules.
    vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
    vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
}

void Renderer::CreateDepthResources()
{
    // Create the depth image and image view.
    CreateImage(vkDevice, vkPhysicalDevice, vkSwapChainWidth, vkSwapChainHeight, vkDepthImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkDepthImage, vkDepthImageMemory);
    CreateImageView(vkDevice, vkDepthImage, vkDepthImageFormat, VK_IMAGE_ASPECT_DEPTH_BIT, vkDepthImageView);
}

void Renderer::CreateFramebuffers()
{
    vkSwapChainFramebuffers.resize(vkSwapChainImageViews.size());

    for (size_t i = 0; i < vkSwapChainImageViews.size(); i++)
    {
        std::array<VkImageView, 2> attachments = {
            vkSwapChainImageViews[i],
            vkDepthImageView
        };
        
        // Create the current framebuffer creation information.
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = vkRenderPass;
        framebufferInfo.attachmentCount = (uint32_t)attachments.size();
        framebufferInfo.pAttachments    = attachments.data();
        framebufferInfo.width           = vkSwapChainWidth;
        framebufferInfo.height          = vkSwapChainHeight;
        framebufferInfo.layers          = 1;

        // Create the current framebuffer.
        if (vkCreateFramebuffer(vkDevice, &framebufferInfo, nullptr, &vkSwapChainFramebuffers[i]) != VK_SUCCESS) {
            std::cout << "ERROR (Vulkan): Failed to create framebuffer." << std::endl;
            throw std::runtime_error("VULKAN_FRAMEBUFFER_ERROR");
        }
    }
}

void Renderer::CreateCommandPool()
{
    const QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(vkPhysicalDevice, vkSurface);

    // Set the command pool creation information.
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    // Create the command pool.
    if (vkCreateCommandPool(vkDevice, &poolInfo, nullptr, &vkCommandPool) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to create command pool." << std::endl;
        throw std::runtime_error("VULKAN_COMMAND_POOL_ERROR");
    }
}

void Renderer::CreateTextureSampler()
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(vkPhysicalDevice, &properties);
    
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter               = VK_FILTER_LINEAR;
    samplerInfo.minFilter               = VK_FILTER_LINEAR;
    samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable        = VK_TRUE;
    samplerInfo.maxAnisotropy           = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable           = VK_FALSE;
    samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias              = 0.f;
    samplerInfo.minLod                  = 0.f;
    samplerInfo.maxLod                  = 0.f;
    if (vkCreateSampler(vkDevice, &samplerInfo, nullptr, &vkTextureSampler) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to create texture sampler." << std::endl;
        throw std::runtime_error("VULKAN_TEXTURE_SAMPLER_ERROR");
    }
}

void Renderer::CreateVertexBuffer()
{
    // TODO: This probably shouldn't be done here, but for every mesh...
    // Create a temporary staging buffer.
    const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    // Map the buffer's GPU memory to CPU memory, and write vertex info to it.
    void* data;
    vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(vkDevice, stagingBufferMemory);

    // Create the vertex buffer.
    CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 vkVertexBuffer, vkVertexBufferMemory);

    // Copy the staging buffer to the vertex buffer.
    CopyBuffer(vkDevice, vkCommandPool, vkGraphicsQueue, stagingBuffer, vkVertexBuffer, bufferSize);

    // De-allocate the staging buffer.
    vkDestroyBuffer(vkDevice, stagingBuffer,       nullptr);
    vkFreeMemory   (vkDevice, stagingBufferMemory, nullptr);
}

void Renderer::CreateIndexBuffer()
{
    // TODO: This probably shouldn't be done here, but for every mesh...
    // Create a temporary staging buffer.
    const VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    // Map the buffer's GPU memory to CPU memory, and write vertex info to it.
    void* data;
    vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(vkDevice, stagingBufferMemory);

    // Create the vertex buffer.
    CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 vkIndexBuffer, vkIndexBufferMemory);

    // Copy the staging buffer to the vertex buffer.
    CopyBuffer(vkDevice, vkCommandPool, vkGraphicsQueue, stagingBuffer, vkIndexBuffer, bufferSize);

    // De-allocate the staging buffer.
    vkDestroyBuffer(vkDevice, stagingBuffer,       nullptr);
    vkFreeMemory   (vkDevice, stagingBufferMemory, nullptr);
}

void Renderer::CreateUniformBuffers()
{
    const VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    vkUniformBuffers      .resize(MAX_FRAMES_IN_FLIGHT);
    vkUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    vkUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     vkUniformBuffers[i], vkUniformBuffersMemory[i]);

        vkMapMemory(vkDevice, vkUniformBuffersMemory[i], 0, bufferSize, 0, &vkUniformBuffersMapped[i]);
    }
}

void Renderer::CreateDescriptorPool()
{
    // Set the type and number of descriptors.
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;
    poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;

    // Create the descriptor pool.
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
    poolInfo.pPoolSizes    = poolSizes.data();
    poolInfo.maxSets       = (uint32_t)MAX_FRAMES_IN_FLIGHT;
    if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &vkDescriptorPool) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to create descriptor pool." << std::endl;
        throw std::runtime_error("VULKAN_DESCRIPTOR_POOL_ERROR");
    }
}

void Renderer::CreateDescriptorSets()
{
    // Create the descriptor sets.
    const std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, vkDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = vkDescriptorPool;
    allocInfo.descriptorSetCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts        = layouts.data();
    vkDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(vkDevice, &allocInfo, vkDescriptorSets.data()) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to allocate descriptor sets." << std::endl;
        throw std::runtime_error("VULKAN_DESCRIPTOR_SETS_ERROR");
    }

    // Populate the descriptor sets.
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = vkUniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range  = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView   = texture->GetVkImageView();
        imageInfo.sampler     = vkTextureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet           = vkDescriptorSets[i];
        descriptorWrites[0].dstBinding       = 0;
        descriptorWrites[0].dstArrayElement  = 0;
        descriptorWrites[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount  = 1;
        descriptorWrites[0].pBufferInfo      = &bufferInfo;

        descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet          = vkDescriptorSets[i];
        descriptorWrites[1].dstBinding      = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo      = &imageInfo;
        
        vkUpdateDescriptorSets(vkDevice, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}

void Renderer::CreateCommandBuffers()
{
    // Set the command buffer allocation information.
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = vkCommandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;

    // Allocate the command buffer.
    vkCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateCommandBuffers(vkDevice, &allocInfo, vkCommandBuffers.data()) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to allocate command buffers." << std::endl;
        throw std::runtime_error("VULKAN_COMMAND_BUFFER_ERROR");
    }
}

void Renderer::CreateSyncObjects()
{
    vkImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    vkRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    vkInFlightFences          .resize(MAX_FRAMES_IN_FLIGHT);
    
    // Set the semaphore and fence creation information.
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Create the semaphores and fence.
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &vkImageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &vkRenderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence    (vkDevice, &fenceInfo,     nullptr, &vkInFlightFences[i])           != VK_SUCCESS)
        {
            std::cout << "ERROR (Vulkan): Failed to create semaphores and fence." << std::endl;
            throw std::runtime_error("VULKAN_SYNC_OBJECTS_ERROR");
        }
    }
}
#pragma endregion

#pragma region Swapchain Recreation
void Renderer::DestroySwapChain() const
{
    for (const VkFramebuffer& vkSwapChainFramebuffer : vkSwapChainFramebuffers)
        vkDestroyFramebuffer(vkDevice, vkSwapChainFramebuffer, nullptr);
    for (const VkImageView& vkSwapChainImageView : vkSwapChainImageViews)
        vkDestroyImageView(vkDevice, vkSwapChainImageView,     nullptr);
    vkDestroyImageView    (vkDevice, vkDepthImageView,         nullptr);
    vkDestroyImage        (vkDevice, vkDepthImage,             nullptr);
    vkFreeMemory          (vkDevice, vkDepthImageMemory,       nullptr);
    vkDestroySwapchainKHR (vkDevice, vkSwapChain,              nullptr);
}

void Renderer::RecreateSwapChain()
{    
    vkDeviceWaitIdle(vkDevice);
    DestroySwapChain();
    
    CreateSwapChain();
    CreateImageViews();
    CreateDepthResources();
    CreateFramebuffers();
}
#pragma endregion 

void Renderer::UpdateUniformBuffer() const
{
    // Get the current time.
    static auto  startTime   = std::chrono::high_resolution_clock::now();
    const  auto  currentTime = std::chrono::high_resolution_clock::now();
    const  float time        = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    // Create the model view proj matrices.
    UniformBufferObject ubo{};
    ubo.model = Maths::Mat4::FromAngleAxis({ time * PI * 0.1f, { 0, 0, 1 } }) * Maths::Mat4::FromAngleAxis({ PI * 0.5f, { 1, 0, 0 } });
    ubo.view  = Maths::Mat4::FromTranslation({ 0, 0.7f, -0.7f }) * Maths::Mat4::FromAngleAxis({ -PI * 0.2f, { 1, 0, 0 } });
    const float aspect = (float)vkSwapChainWidth / (float)vkSwapChainHeight;
    const float yScale = 1 / tan(Maths::degToRad(80) * 0.5f);
    const float xScale = 1 / (yScale * aspect);
    ubo.proj = Maths::Mat4(
        xScale, 0, 0, 0,
        0, yScale, 0, 0,
        0, 0, -(500 + 0.1f) / (500 - 0.1f), -1,
        0, 0, -(2 * 500 * 0.1f) / (500 - 0.1f), 1
    );

    // Copy the matrices to buffer memory.
    memcpy(vkUniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

#pragma region Rendering
void Renderer::NewFrame()
{
    // Wait for the previous frame to finish.
    vkWaitForFences(vkDevice, 1, &vkInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // Acquire an image from the swap chain.
    const VkResult result = vkAcquireNextImageKHR(vkDevice, vkSwapChain, UINT64_MAX, vkImageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &vkSwapchainImageIndex);

    // Recreate the swap chain if it is out of date.
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain();
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cout << "ERROR (Vulkan): Failed to acquire swap chain image." << std::endl;
        throw std::runtime_error("VULKAN_IMAGE_ACQUISITION_ERROR");
    }

    // Only reset fences if we are going to be submitting work.
    vkResetFences(vkDevice, 1, &vkInFlightFences[currentFrame]);
}

void Renderer::BeginRecordCmdBuf() const
{
    // Reset the command buffer.
    vkResetCommandBuffer(vkCommandBuffers[currentFrame],  0);
    
    // Begin recording the command buffer.
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags            = 0;       // Optional.
    beginInfo.pInheritanceInfo = nullptr; // Optional.
    if (vkBeginCommandBuffer(vkCommandBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to begin recording command buffer." << std::endl;
        throw std::runtime_error("VULKAN_BEGIN_COMMAND_BUFFER_ERROR");
    }

    // Define the clear color.
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    clearValues[1].depthStencil = { 1.0f, 0 };

    // Begin the render pass.
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = vkRenderPass;
    renderPassInfo.framebuffer       = vkSwapChainFramebuffers[vkSwapchainImageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = { vkSwapChainWidth, vkSwapChainHeight };
    renderPassInfo.clearValueCount   = (uint32_t)clearValues.size();
    renderPassInfo.pClearValues      = clearValues.data();
    vkCmdBeginRenderPass(vkCommandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind the graphics pipeline.
    vkCmdBindPipeline(vkCommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, vkGraphicsPipeline);

    // Bind the vertex and index buffers.
    const VkBuffer vertexBuffers[] = { vkVertexBuffer };
    const VkDeviceSize offsets[]   = { 0 };
    vkCmdBindVertexBuffers(vkCommandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer  (vkCommandBuffers[currentFrame], vkIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

    // Set the viewport.
    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (float)vkSwapChainWidth;
    viewport.height   = (float)vkSwapChainHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(vkCommandBuffers[currentFrame], 0, 1, &viewport);

    // Set the scissor.
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { vkSwapChainWidth, vkSwapChainHeight };
    vkCmdSetScissor(vkCommandBuffers[currentFrame], 0, 1, &scissor);
}

void Renderer::EndRecordCmdBuf() const
{
    // End the render pass.
    vkCmdEndRenderPass(vkCommandBuffers[currentFrame]);

    // Stop recording the command buffer.
    if (vkEndCommandBuffer(vkCommandBuffers[currentFrame]) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to record command buffer." << std::endl;
        throw std::runtime_error("VULKAN_RECORD_COMMAND_BUFFER_ERROR");
    }
}

void Renderer::PresentFrame()
{
    // Set the command buffer submit information.
    const VkSemaphore          waitSemaphores  [] = { vkImageAvailableSemaphores[currentFrame] };
    const VkPipelineStageFlags waitStages      [] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    const VkSemaphore          signalSemaphores[] = { vkRenderFinishedSemaphores[currentFrame] };
    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &vkCommandBuffers[currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    // Submit the graphics command queue.
    if (vkQueueSubmit(vkGraphicsQueue, 1, &submitInfo, vkInFlightFences[currentFrame]) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to submit draw command buffer." << std::endl;
        throw std::runtime_error("VULKAN_SUBMIT_COMMAND_BUFFER_ERROR");
    }

    // Present the frame.
    const VkSwapchainKHR swapChains[] = { vkSwapChain };
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = swapChains;
    presentInfo.pImageIndices      = &vkSwapchainImageIndex;
    presentInfo.pResults           = nullptr; // Optional.
    const VkResult result = vkQueuePresentKHR(vkPresentQueue, &presentInfo);

    // Recreate the swap chain if it is out of date or suboptimal.
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        RecreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to present swap chain image." << std::endl;
        throw std::runtime_error("VULKAN_PRESENTATION_ERROR");
    }

    // Move to the next frame.
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
#pragma endregion
