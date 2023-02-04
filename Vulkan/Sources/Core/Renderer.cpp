#include "Core/Renderer.h"
#include "Core/Application.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <set>
#include <limits>
#include <algorithm>
#include <fstream>
#include <iostream>
using namespace Core;
using namespace VulkanUtils;


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
    // TODO: Choose the best GPU (see: https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families).
    return FindQueueFamilies(device, surface).IsComplete()
        && CheckDeviceExtensionSupport(device)
        && QuerySwapChainSupport(device, surface).IsAdequate();
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

VkShaderModule VulkanUtils::CreateShaderModule(const VkDevice& device, const char* filename)
{
    // Read the shader code.
    const std::vector<char> shaderCode = ReadBinFile(filename);
    
    // Set shader module creation information.
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(shaderCode.data());

    // Create and return the shader module.
    VkShaderModule shaderModule{};
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to create shader module." << std::endl;
        throw std::runtime_error("VULKAN_SHADER_MODULE_ERROR");
    }
    return shaderModule;
}
#pragma endregion


Renderer::Renderer(const char* appName, const char* engineName)
{
    app = Application::Get();
    vkPhysicalDevice = VK_NULL_HANDLE;
    vkSwapChainImageFormat = VK_FORMAT_UNDEFINED;

    CheckValidationLayers();
    CreateVkInstance(appName, engineName);
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFramebuffers();
    CreateCommandPool();
    CreateCommandBuffer();
    CreateSyncObjects();
}

Renderer::~Renderer()
{
    vkDeviceWaitIdle    (vkDevice);
    vkDestroyFence      (vkDevice, vkInFlightFence,           nullptr);
    vkDestroySemaphore  (vkDevice, vkRenderFinishedSemaphore, nullptr);
    vkDestroySemaphore  (vkDevice, vkImageAvailableSemaphore, nullptr);
    vkDestroyCommandPool(vkDevice, vkCommandPool,             nullptr);
    for (const VkFramebuffer& framebuffer : vkSwapChainFramebuffers)
        vkDestroyFramebuffer(vkDevice, framebuffer,        nullptr);
    vkDestroyPipeline       (vkDevice, vkGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout (vkDevice, vkPipelineLayout,   nullptr);
    vkDestroyRenderPass     (vkDevice, vkRenderPass,       nullptr);
    for (const VkImageView& imageView : vkSwapChainImageViews)
        vkDestroyImageView(vkDevice, imageView,   nullptr);
    vkDestroySwapchainKHR (vkDevice, vkSwapChain, nullptr);
    vkDestroyDevice       (vkDevice,              nullptr);
    vkDestroySurfaceKHR   (vkInstance, vkSurface, nullptr);
    vkDestroyInstance     (vkInstance,            nullptr);
}

void Renderer::BeginRender()
{
    NewFrame();
    BeginRecordCmdBuf();
}

void Renderer::DrawFrame() const
{
    // Issue the command to draw the triangle.
    vkCmdDraw(vkCommandBuffer, 3, 1, 0, 0);
}

void Renderer::EndRender() const
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
    appInfo.apiVersion         = VK_API_VERSION_1_0;

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
    {
        // Set creation information for the image view.
        VkImageViewCreateInfo createInfo{};
        createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image    = vkSwapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format   = vkSwapChainImageFormat;

        // Set color channel swizzling.
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // Choose how the image is used and set the mipmap and layer counts.
        createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel   = 0;
        createInfo.subresourceRange.levelCount     = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount     = 1;

        // Create the image view.
        if (vkCreateImageView(vkDevice, &createInfo, nullptr, &vkSwapChainImageViews[i]) != VK_SUCCESS) {
            std::cout << "ERROR (Vulkan): Failed to create image views." << std::endl;
            throw std::runtime_error("VULKAN_IMAGE_VIEW_ERROR");
        }
    }
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

    // Create a single sub-pass.
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorAttachmentRef;

    // Create a sub-pass dependency.
    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Set the render pass creation information.
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments    = &colorAttachment;
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

void Renderer::CreateGraphicsPipeline()
{
    // Load vulkan fragment and vertex shaders.
    VkShaderModule vertShaderModule = CreateShaderModule(vkDevice, "Shaders/VulkanVert.spv");
    VkShaderModule fragShaderModule = CreateShaderModule(vkDevice, "Shaders/VulkanFrag.spv");

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
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 0;
    vertexInputInfo.pVertexBindingDescriptions      = nullptr; // Optional.
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions    = nullptr; // Optional.

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
    // TODO: Currently don't have one...

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
    pipelineLayoutInfo.setLayoutCount         = 0;       // Optional.
    pipelineLayoutInfo.pSetLayouts            = nullptr; // Optional.
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
    pipelineInfo.pDepthStencilState  = nullptr; // Optional.
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

void Renderer::CreateFramebuffers()
{
    vkSwapChainFramebuffers.resize(vkSwapChainImageViews.size());

    for (size_t i = 0; i < vkSwapChainImageViews.size(); i++)
    {
        // Create the current framebuffer creation information.
        VkFramebufferCreateInfo framebufferInfo{};
        const VkImageView attachments[] = { vkSwapChainImageViews[i] };
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = vkRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments    = attachments;
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

void Renderer::CreateCommandBuffer()
{
    // Set the command buffer allocation information.
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = vkCommandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    // Allocate the command buffer.
    if (vkAllocateCommandBuffers(vkDevice, &allocInfo, &vkCommandBuffer) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to allocate command buffers." << std::endl;
        throw std::runtime_error("VULKAN_COMMAND_BUFFER_ERROR");
    }
}

void Renderer::CreateSyncObjects()
{
    // Set the semaphore and fence creation information.
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Create the semaphores and fence.
    if (vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &vkImageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &vkRenderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence    (vkDevice, &fenceInfo,     nullptr, &vkInFlightFence          ) != VK_SUCCESS)
    {
        std::cout << "ERROR (Vulkan): Failed to create semaphores." << std::endl;
        throw std::runtime_error("VULKAN_SEMAPHORE_ERROR");
    }
}
#pragma endregion

#pragma region Rendering
void Renderer::NewFrame()
{
    // Wait for the previous frame to finish.
    vkWaitForFences(vkDevice, 1, &vkInFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences  (vkDevice, 1, &vkInFlightFence);

    // Acquire an image from the swap chain.
    vkAcquireNextImageKHR(vkDevice, vkSwapChain, UINT64_MAX, vkImageAvailableSemaphore, VK_NULL_HANDLE, &vkSwapchainImageIndex);

    // Reset the command buffer and start recording it.
    vkResetCommandBuffer(vkCommandBuffer, 0);
}

void Renderer::BeginRecordCmdBuf() const
{
    vkResetCommandBuffer(vkCommandBuffer, 0);
    
    // Begin recording the command buffer.
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags            = 0;       // Optional.
    beginInfo.pInheritanceInfo = nullptr; // Optional.
    if (vkBeginCommandBuffer(vkCommandBuffer, &beginInfo) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to begin recording command buffer." << std::endl;
        throw std::runtime_error("VULKAN_BEGIN_COMMAND_BUFFER_ERROR");
    }

    // Define the clear color.
    const VkClearValue clearColor = {{{ 0.0f, 0.0f, 0.0f, 1.0f }}};

    // Begin the render pass.
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = vkRenderPass;
    renderPassInfo.framebuffer       = vkSwapChainFramebuffers[vkSwapchainImageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = { vkSwapChainWidth, vkSwapChainHeight };
    renderPassInfo.clearValueCount   = 1;
    renderPassInfo.pClearValues      = &clearColor;
    vkCmdBeginRenderPass(vkCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind the graphics pipeline.
    vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkGraphicsPipeline);

    // Set the viewport.
    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (float)vkSwapChainWidth;
    viewport.height   = (float)vkSwapChainHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(vkCommandBuffer, 0, 1, &viewport);

    // Set the scissor.
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { vkSwapChainWidth, vkSwapChainHeight };
    vkCmdSetScissor(vkCommandBuffer, 0, 1, &scissor);
}

void Renderer::EndRecordCmdBuf() const
{
    // End the render pass.
    vkCmdEndRenderPass(vkCommandBuffer);

    // Stop recording the command buffer.
    if (vkEndCommandBuffer(vkCommandBuffer) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to record command buffer." << std::endl;
        throw std::runtime_error("VULKAN_RECORD_COMMAND_BUFFER_ERROR");
    }
}

void Renderer::PresentFrame() const
{
    // Set the command buffer submit information.
    const VkSemaphore          waitSemaphores  [] = { vkImageAvailableSemaphore };
    const VkPipelineStageFlags waitStages      [] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    const VkSemaphore          signalSemaphores[] = { vkRenderFinishedSemaphore };
    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &vkCommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    // Submit the graphics command queue.
    if (vkQueueSubmit(vkGraphicsQueue, 1, &submitInfo, vkInFlightFence) != VK_SUCCESS) {
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
    vkQueuePresentKHR(vkPresentQueue, &presentInfo);
}
#pragma endregion
