#include "Core/Renderer.h"
#include "Core/Application.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
using namespace Core;

// Define which validation layers should be used.
const std::vector VK_VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"
};

// Use validation layers only in debug mode.
#ifdef NDEBUG
    constexpr bool VK_VALIDATION_LAYERS_ENABLED = false;
#else
    constexpr bool VK_VALIDATION_LAYERS_ENABLED = true;
#endif


Renderer::Renderer(const char* appName, const char* engineName)
{
    app = Application::Get();
    vkInstance = nullptr;
    vkPhysicalDevice = VK_NULL_HANDLE;

    CheckValidationLayers();
    CreateVkInstance(appName, engineName);
    PickPhysicalDevice();
    CreateLogicalDevice();
}

Renderer::~Renderer()
{
    vkDestroyDevice  (vkDevice,   nullptr);
    vkDestroyInstance(vkInstance, nullptr);
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
    for (const char* layerName : VK_VALIDATION_LAYERS)
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
    if (VK_VALIDATION_LAYERS_ENABLED && validationLayersError) {
        std::cout << "ERROR (Vulkan): Some of the requested validation layers are not available.";
        throw std::runtime_error("VK_VALIDATION_LAYERS_ERROR");
    }
}

void Renderer::CreateVkInstance(const char* appName, const char* engineName)
{
    // Set application information.
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = appName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = engineName;
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Set vulkan instance creation information.
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Set validation layer information.
    if (VK_VALIDATION_LAYERS_ENABLED) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VK_VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VK_VALIDATION_LAYERS.data();
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
        if (IsDeviceSuitable(device)) {
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
    const VkQueueFamilyIndices indices = FindQueueFamilies(vkPhysicalDevice);

    // Choose which queues should be created.
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
    queueCreateInfo.queueCount = 1;

    // Set queue priority.
    const float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    // Specify the devices features to be used.
    VkPhysicalDeviceFeatures deviceFeatures{};

    // Set the logical device creation information.
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 0;
    if (VK_VALIDATION_LAYERS_ENABLED) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VK_VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VK_VALIDATION_LAYERS.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // Create the logical device.
    if (vkCreateDevice(vkPhysicalDevice, &createInfo, nullptr, &vkDevice) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to create a logical device." << std::endl;
        throw std::runtime_error("VULKAN_LOGICAL_DEVICE_ERROR");
    }

    // Get the graphics queue handle.
    vkGetDeviceQueue(vkDevice, indices.graphicsFamily.value(), 0, &vkGraphicsQueue);
}

VkQueueFamilyIndices Renderer::FindQueueFamilies(const VkPhysicalDevice& device) const
{
    VkQueueFamilyIndices queueFamilyIndices;

    // Get the number of queue families on the given GPU.
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    // Get an array of all the queue families on the given GPU.
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    // Get all queue family indices.
    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
            queueFamilyIndices.graphicsFamily = i;
        if (queueFamilyIndices.IsComplete())
            break;
        ++i;
    }
    return queueFamilyIndices;
}

bool Renderer::IsDeviceSuitable(const VkPhysicalDevice& device)
{
    // TODO: Choose the best GPU instead of the first (see: https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families).
    return FindQueueFamilies(device).IsComplete();
}
