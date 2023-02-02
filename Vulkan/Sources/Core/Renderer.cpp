#include "Core/Renderer.h"
#include "Core/Application.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <set>
#include <limits>
#include <algorithm>
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
#pragma endregion


Renderer::Renderer(const char* appName, const char* engineName)
{
    app = Application::Get();
    vkPhysicalDevice = VK_NULL_HANDLE;

    CheckValidationLayers();
    CreateVkInstance(appName, engineName);
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
}

Renderer::~Renderer()
{
    vkDestroySwapchainKHR(vkDevice, vkSwapChain, nullptr);
    vkDestroyDevice      (vkDevice, nullptr);
    vkDestroySurfaceKHR  (vkInstance, vkSurface, nullptr);
    vkDestroyInstance    (vkInstance, nullptr);
}

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
        std::cout << "ERROR (Vulkan): Some of the requested validation layers are not available.";
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
