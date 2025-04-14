#include "Core/Renderer.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Core/Window.h"
#include "Resources/Camera.h"
#include "Resources/Model.h"
#include "Resources/Mesh.h"
#include "Resources/Texture.h"
#include "Resources/Light.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <set>
#include <array>
#include <algorithm>
#include <fstream>

#include "Core/GpuDataManager.h"
#include "Core/RendererShadows.h"
using namespace Core;
using namespace GraphicsUtils;

Renderer::Renderer(Application* application, const char* appName, const char* engineName, bool vsync)
{
    app     = application;
    gpuData = app->GetGpuData();
    gpuData->SetRendererPtr(this);
    
    vkPhysicalDevice       = VK_NULL_HANDLE;
    vkDepthImageFormat     = VK_FORMAT_UNDEFINED;
    vkSwapChainImageFormat = VK_FORMAT_UNDEFINED;
    msaaSamples            = VK_SAMPLE_COUNT_1_BIT;

    CheckValidationLayers();
    CreateVkInstance(appName, engineName);
    CreateDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain(vsync);
    CreateImageViews();
    vkDepthImageFormat = FindSupportedFormat(vkPhysicalDevice,
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
    CreateRenderPass();
    CreateDescriptorLayoutsAndPools();
    rendererShadows = new RendererShadows(app, this, 2048, 2048);
    CreateGraphicsPipeline();
    CreateColorResources();
    CreateDepthResources();
    CreateFramebuffers();
    CreateCommandPool();
    CreateTextureSampler();
    CreateCommandBuffers();
    CreateSyncObjects();
    SetDistanceFogParams(0, 60, 100);
}

Renderer::~Renderer()
{
    WaitUntilIdle();
    const auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT");
    
    vkDestroyBuffer(vkDevice, fogParamsBuffer,       nullptr);
    vkFreeMemory   (vkDevice, fogParamsBufferMemory, nullptr);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vkDevice, vkRenderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(vkDevice, vkImageAvailableSemaphores[i], nullptr);
        vkDestroyFence    (vkDevice, vkInFlightFences[i],           nullptr);
    }
    DestroySwapChain();
    DestroyDescriptorLayoutsAndPools();
    vkDestroySampler               (vkDevice, vkTextureSampler,   nullptr);
    vkDestroyCommandPool           (vkDevice, vkCommandPool,      nullptr);
    vkDestroyPipeline              (vkDevice, vkGraphicsPipeline, nullptr);
    delete rendererShadows;
    vkDestroyPipelineLayout        (vkDevice, vkPipelineLayout,   nullptr);
    vkDestroyRenderPass            (vkDevice, vkRenderPass,       nullptr);
    vkDestroyDevice                (vkDevice,                     nullptr);
    vkDestroySurfaceKHR            (vkInstance, vkSurface,        nullptr);
    vkDestroyDebugUtilsMessengerEXT(vkInstance, vkDebugMessenger, nullptr);
    vkDestroyInstance              (vkInstance,                   nullptr);
}

template<ShaderStage S> void Renderer::SetShaderFrameConstants(const ShaderFrameConstants<S>& constants)
{
    vkCmdPushConstants(GetCurVkCommandBuffer(), GetVkPipelineLayout(), ShaderStageToFlagBits(S), 0, sizeof(ShaderFrameConstants<S>), &constants);
}
template void Renderer::SetShaderFrameConstants(const ShaderFrameConstants<ShaderStage::Vertex>&);
template void Renderer::SetShaderFrameConstants(const ShaderFrameConstants<ShaderStage::TessellationControl>&);
template void Renderer::SetShaderFrameConstants(const ShaderFrameConstants<ShaderStage::TessellationEvaluation>&);
template void Renderer::SetShaderFrameConstants(const ShaderFrameConstants<ShaderStage::Geometry>&);
template void Renderer::SetShaderFrameConstants(const ShaderFrameConstants<ShaderStage::Fragment>&);
template void Renderer::SetShaderFrameConstants(const ShaderFrameConstants<ShaderStage::Compute>&);
// Above is a hack to define templates in source files and force the compiler to link all specifications.

void Renderer::SetDistanceFogParams(const Maths::RGB& color, const float& start, const float& end)
{
    const DistanceFogParams fogParams{ color, start, end, 1 / (end - start) };
    
    // Create a temporary staging buffer.
    constexpr VkDeviceSize bufferSize = sizeof(DistanceFogParams);
    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    // Map the buffer's GPU memory to CPU memory, and write buffer params to it.
    void* data;
    vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, &fogParams, (size_t)bufferSize);
    vkUnmapMemory(vkDevice, stagingBufferMemory);

    // De-allocate any previously created fog params buffer.
    if (fogParamsBuffer && fogParamsBufferMemory)
    {
        vkDestroyBuffer(vkDevice, fogParamsBuffer,       nullptr);
        vkFreeMemory   (vkDevice, fogParamsBufferMemory, nullptr);
    }

    // Create the real fog params buffer.
    CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 fogParamsBuffer, fogParamsBufferMemory);

    // Copy the staging buffer to the fog params buffer.
    CopyBuffer(vkDevice, vkCommandPool, vkGraphicsQueue, stagingBuffer, fogParamsBuffer, bufferSize);

    // De-allocate the staging buffer.
    vkDestroyBuffer(vkDevice, stagingBuffer,       nullptr);
    vkFreeMemory   (vkDevice, stagingBufferMemory, nullptr);
    
    // Populate the descriptor set.
    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = fogParamsBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range  = sizeof(DistanceFogParams);
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet          = constDataDescriptorSet;
    descriptorWrite.dstBinding      = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo     = &bufferInfo;
    vkUpdateDescriptorSets(vkDevice, 1, &descriptorWrite, 0, nullptr);
}

void Renderer::BeginRender()
{
    NewFrame();
    BeginRenderPass();
}

void Renderer::DrawModel(const Resources::Model& model, const Resources::Camera& camera) const
{
    // Get the light array as well as the model's GPU data and update it.
    const GpuArray<Resources::Light>& lightArray = gpuData->GetArray<Resources::Light>();
    const GpuData <Resources::Model>& modelData  = gpuData->GetData(model);
    model.UpdateMvpBuffer(camera, currentFrame, &modelData); // TODO: This should be done elsewhere, before the render.

    // Draw each of the model's meshes one by one.
    const std::vector<Resources::Mesh>& meshes = model.GetMeshes();
    for (const Resources::Mesh& mesh : meshes)
    {
        // Get the mesh and material GPU data.
        const GpuData<Resources::Mesh>&     meshData     = gpuData->GetData(mesh);
        const GpuData<Resources::Material>& materialData = gpuData->GetData(*mesh.GetMaterial());
        
        // Bind the vertex and index buffers.
        const VkBuffer vertexBuffer = meshData.vkVertexBuffer;
        const VkBuffer indexBuffer  = meshData.vkIndexBuffer ;
        constexpr VkDeviceSize vertexOffset = 0;
        vkCmdBindVertexBuffers(vkCommandBuffers[currentFrame], 0, 1, &vertexBuffer, &vertexOffset);
        vkCmdBindIndexBuffer  (vkCommandBuffers[currentFrame], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // Bind the descriptor sets and draw.
        const VkDescriptorSet descriptorSets[5] = { modelData.vkDescriptorSets[currentFrame], rendererShadows->GetVkDescriptorSet(), constDataDescriptorSet, materialData.vkDescriptorSet, lightArray.vkDescriptorSet };
        constexpr uint32_t dynamicOffset = 0;
        vkCmdBindDescriptorSets(vkCommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineLayout, 0, 5, descriptorSets, 1, &dynamicOffset);
        vkCmdDrawIndexed(vkCommandBuffers[currentFrame], mesh.GetIndexCount(), 1, 0, 0, 0);
    }
}

void Renderer::EndRender()
{
    EndRenderPass();
    PresentFrame();
}

void Renderer::WaitUntilIdle() const
{
    vkDeviceWaitIdle(vkDevice);
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
        LogError(LogType::Vulkan, "Some of the requested validation layers are not available.");
        throw std::runtime_error("VULKAN_VALIDATION_LAYERS_ERROR");
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
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    createInfo.enabledExtensionCount   = (uint32_t)extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Create the vulkan instance.
    if (vkCreateInstance(&createInfo, nullptr, &vkInstance) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Unable to create a Vulkan instance.");
        throw std::runtime_error("VULKAN_INIT_ERROR");
    }
}

VkBool32 VkDebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    LogSeverity logSeverity;
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        logSeverity = LogSeverity::Info;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        logSeverity = LogSeverity::Warning;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        logSeverity = LogSeverity::Error;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
    default:
        logSeverity = LogSeverity::None;
        break;
    }

    Logger::PushLog({ LogType::Vulkan, logSeverity, pCallbackData->pMessage, "", "", -1 });
    return false;
}

void Renderer::CreateDebugMessenger()
{
    // Setup debug messenger properties.
    VkDebugUtilsMessengerCreateInfoEXT messengerInfo{};
    messengerInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    messengerInfo.pfnUserCallback = VkDebugMessengerCallback;
    messengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    messengerInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    messengerInfo.pUserData       = nullptr;
    
    const auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT");
    if (vkCreateDebugUtilsMessengerEXT(vkInstance, &messengerInfo, nullptr, &vkDebugMessenger) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Unable to link engine logger to Vulkan log outputs.");
        throw std::runtime_error("VULKAN_LOGGER_INIT_ERROR");
    }
}

void Renderer::CreateSurface()
{
    if (glfwCreateWindowSurface(vkInstance, app->GetWindow()->GetGlfwWindow(), nullptr, &vkSurface) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Unable to create a  window surface.");
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
        LogError(LogType::Vulkan, "Couldn't find a GPU with Vulkan support.");
        throw std::runtime_error("GPU_NO_VULKAN_SUPPORT_ERROR");
    }

    // Get an array of all GPUs with Vulkan support.
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

    // Try to find a suitable GPU to run Vulkan on.
    for (const auto& device : devices) {
        if (IsDeviceSuitable(device, vkSurface)) {
            vkPhysicalDevice = device;
            msaaSamples = GetMaxUsableSampleCount(vkPhysicalDevice);
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);
            LogInfo(LogType::Vulkan, "Using rendering device " + std::string(properties.deviceName));
            break;
        }
    }

    // Make sure a suitable GPU was found.
    if (vkPhysicalDevice == VK_NULL_HANDLE) {
        LogError(LogType::Vulkan, "Couldn't find a GPU with Vulkan support.");
        throw std::runtime_error("GPU_NO_VULKAN_SUPPORT_ERROR");
    }
}

void Renderer::CreateLogicalDevice()
{
    vkQueueFamilyIndices = FindQueueFamilies(vkPhysicalDevice, vkSurface);

    // Set creation information for all required queues.
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    const std::set<uint32_t> uniqueQueueFamilies = { vkQueueFamilyIndices.graphicsFamily.value(), vkQueueFamilyIndices.presentFamily.value() };
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
    deviceFeatures.sampleRateShading = VK_TRUE;
    deviceFeatures.shaderStorageImageExtendedFormats = VK_TRUE;

    // Enable null descriptors.
    VkPhysicalDeviceRobustness2FeaturesEXT deviceRobustnessFeatures{};
    deviceRobustnessFeatures.sType          = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
    deviceRobustnessFeatures.nullDescriptor = VK_TRUE;

    // Enable uniform buffer update after bind.
    VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
    indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    indexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
    indexingFeatures.pNext = &deviceRobustnessFeatures;

    // Set the logical device creation information.
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount    = (uint32_t)queueCreateInfos.size();
    deviceCreateInfo.pQueueCreateInfos       = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures        = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(EXTENSIONS.size());
    deviceCreateInfo.ppEnabledExtensionNames = EXTENSIONS.data();
    deviceCreateInfo.pNext                   = &indexingFeatures;
    if (VALIDATION_LAYERS_ENABLED) {
        deviceCreateInfo.enabledLayerCount   = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        deviceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    } else {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    // Create the logical device.
    if (vkCreateDevice(vkPhysicalDevice, &deviceCreateInfo, nullptr, &vkDevice) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create a logical device.");
        throw std::runtime_error("VULKAN_LOGICAL_DEVICE_ERROR");
    }

    // Get the graphics and present queue handles.
    vkGetDeviceQueue(vkDevice, vkQueueFamilyIndices.graphicsFamily.value(), 0, &vkGraphicsQueue);
    vkGetDeviceQueue(vkDevice, vkQueueFamilyIndices.presentFamily .value(), 0, &vkPresentQueue );
}

void Renderer::CreateSwapChain(const bool vsync)
{
    const SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(vkPhysicalDevice, vkSurface);

    // Get the surface format, presentation mode and extent of the swap chain.
    const VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat( swapChainSupport.formats);
    const VkPresentModeKHR   presentMode   = ChooseSwapPresentMode  ( swapChainSupport.presentModes, vsync);
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
        LogError(LogType::Vulkan, "Failed to create a swap chain.");
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
        CreateImageView(vkDevice, vkSwapChainImages[i], vkSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, vkSwapChainImageViews[i]);
}

void Renderer::CreateRenderPass()
{
    // Set the color buffer attachment.
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = vkSwapChainImageFormat;
    colorAttachment.samples        = msaaSamples;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // No stencil is used so we don't care.
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Set the depth buffer attachment.
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format         = vkDepthImageFormat;
    depthAttachment.samples        = msaaSamples;
    depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Set the color resolve attachment (to display the multisampled color buffer).
    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format         = vkSwapChainImageFormat;
    colorAttachmentResolve.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Create a single sub-pass.
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments     = &colorAttachmentResolveRef;

    // Create a sub-pass dependency.
    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT          | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // Set the render pass creation information.
    const std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
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
        LogError(LogType::Vulkan, "Failed to create Main render pass.");
        throw std::runtime_error("VULKAN_RENDER_PASS_ERROR");
    }
}

void Renderer::CreateDescriptorLayoutsAndPools()
{
    // Create layouts and pools for lights, models and materials.
    gpuData->CreateArray<Resources::Model>();
    gpuData->CreateArray<Resources::Material>();
    gpuData->CreateArray<Resources::Light>();

    // Create layout, pool and descriptor for constant data.
    {
        // Set the binding of the const data buffer.
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding         = 0;
        layoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

        // Create the descriptor set layout.
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings    = &layoutBinding;
        if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &constDataDescriptorLayout) != VK_SUCCESS) {
            LogError(LogType::Vulkan, "Failed to create descriptor set layout.");
            throw std::runtime_error("VULKAN_DESCRIPTOR_SET_LAYOUT_ERROR");
        }

        // Set the type and number of descriptors.
        VkDescriptorPoolSize poolSize;
        poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = 1;

        // Create the descriptor pool.
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes    = &poolSize;
        poolInfo.maxSets       = 1;
        if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &constDataDescriptorPool) != VK_SUCCESS) {
            LogError(LogType::Vulkan, "Failed to create descriptor pool.");
            throw std::runtime_error("VULKAN_DESCRIPTOR_POOL_ERROR");
        }

        // Allocate the descriptor set.
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = constDataDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts        = &constDataDescriptorLayout;
        if (vkAllocateDescriptorSets(vkDevice, &allocInfo, &constDataDescriptorSet) != VK_SUCCESS) {
            LogError(LogType::Vulkan, "Failed to allocate descriptor sets.");
            throw std::runtime_error("VULKAN_DESCRIPTOR_SET_ALLOCATION_ERROR");
        }
    }
}

void Renderer::CreateGraphicsPipeline()
{
    // Setup vertex bindings and attributes.
    auto bindingDescription    = Resources::Mesh::GetVertexBindingDescription();
    auto attributeDescriptions = Resources::Mesh::GetVertexAttributeDescriptions();
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
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.f; // Optional.
    rasterizer.depthBiasClamp          = 0.f; // Optional.
    rasterizer.depthBiasSlopeFactor    = 0.f; // Optional.

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

    // Load vulkan fragment and vertex shaders.
    VkShaderModule vertShaderModule = CreateShaderModule(vkDevice, ShaderStage::Vertex,   "Shaders/Main.vert");
    VkShaderModule fragShaderModule = CreateShaderModule(vkDevice, ShaderStage::Fragment, "Shaders/Main.frag");

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

    // Set multisampling parameters.
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples  = msaaSamples;
    multisampling.sampleShadingEnable   = VK_TRUE;
    multisampling.minSampleShading      = .2f;
    multisampling.pSampleMask           = nullptr;  // Optional.
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional.
    multisampling.alphaToOneEnable      = VK_FALSE; // Optional.

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

    // Define 1 push constant (viewPos) and 3 descriptor set layouts (model, material, lights).
    const VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Maths::Vector3) };
    const VkDescriptorSetLayout setLayouts[5] = {
        gpuData->GetArray<Resources::Model>().vkDescriptorSetLayout,
        rendererShadows->GetVkDescriptorSetLayout(),
        constDataDescriptorLayout,
        gpuData->GetArray<Resources::Material>().vkDescriptorSetLayout,
        gpuData->GetArray<Resources::Light>().vkDescriptorSetLayout,
    };

    // Set the pipeline layout creation information.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = 5;
    pipelineLayoutInfo.pSetLayouts            = setLayouts;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

    // Create the pipeline layout.
    if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &vkPipelineLayout) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create pipeline layout.");
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

    // Create the graphics pipeline.
    if (vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkGraphicsPipeline) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create graphics pipeline.");
        throw std::runtime_error("VULKAN_GRAPHICS_PIPELINE_ERROR");
    }

    // Destroy both shader modules.
    vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
    vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
}

void Renderer::CreateColorResources()
{
    // Create the color image and image view.
    CreateImage(vkDevice, vkPhysicalDevice, vkSwapChainWidth, vkSwapChainHeight, 1, msaaSamples, vkSwapChainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkColorImage, vkColorImageMemory);
    CreateImageView(vkDevice, vkColorImage, vkSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, vkColorImageView);
}

void Renderer::CreateDepthResources()
{
    // Create the depth image and image view.
    CreateImage(vkDevice, vkPhysicalDevice, vkSwapChainWidth, vkSwapChainHeight, 1, msaaSamples, vkDepthImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkDepthImage, vkDepthImageMemory);
    CreateImageView(vkDevice, vkDepthImage, vkDepthImageFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, vkDepthImageView);
}

void Renderer::CreateFramebuffers()
{
    vkSwapChainFramebuffers.resize(vkSwapChainImageViews.size());

    for (size_t i = 0; i < vkSwapChainImageViews.size(); i++)
    {
        std::array<VkImageView, 3> attachments = {
            vkColorImageView,
            vkDepthImageView,
            vkSwapChainImageViews[i]
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
            LogError(LogType::Vulkan, "Failed to create framebuffer.");
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
        LogError(LogType::Vulkan, "Failed to create command pool.");
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
    samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable        = VK_TRUE;
    samplerInfo.maxAnisotropy           = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable           = VK_FALSE;
    samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    samplerInfo.minLod                  = 0.f;
    samplerInfo.maxLod                  = (float)std::floor(std::log2(std::max(vkSwapChainWidth, vkSwapChainHeight))) + 1; // (float)texture->GetMipLevels();
    if (vkCreateSampler(vkDevice, &samplerInfo, nullptr, &vkTextureSampler) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create texture sampler.");
        throw std::runtime_error("VULKAN_TEXTURE_SAMPLER_ERROR");
    }
}

void Renderer::CreateCommandBuffers()
{
    // Set the command buffer allocation information.
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = vkCommandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    // Allocate the command buffer.
    vkCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateCommandBuffers(vkDevice, &allocInfo, vkCommandBuffers.data()) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to allocate command buffers.");
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
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &vkImageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &vkRenderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence    (vkDevice, &fenceInfo,     nullptr, &vkInFlightFences[i])           != VK_SUCCESS)
        {
            LogError(LogType::Vulkan, "Failed to create semaphores and fence.");
            throw std::runtime_error("VULKAN_SYNC_OBJECTS_ERROR");
        }
    }
}
#pragma endregion

#pragma region Recreation & Destruction
void Renderer::RecreateSwapChain()
{    
    WaitUntilIdle();
    DestroySwapChain();
    
    CreateSwapChain(app->GetWindow()->GetVsync());
    CreateImageViews();
    CreateColorResources();
    CreateDepthResources();
    CreateFramebuffers();
}

void Renderer::DestroySwapChain() const
{
    for (const VkFramebuffer& vkSwapChainFramebuffer : vkSwapChainFramebuffers)
        vkDestroyFramebuffer(vkDevice, vkSwapChainFramebuffer, nullptr);
    for (const VkImageView& vkSwapChainImageView : vkSwapChainImageViews)
        vkDestroyImageView(vkDevice, vkSwapChainImageView,     nullptr);
    vkDestroyImageView    (vkDevice, vkDepthImageView,         nullptr);
    vkDestroyImage        (vkDevice, vkDepthImage,             nullptr);
    vkFreeMemory          (vkDevice, vkDepthImageMemory,       nullptr);
    vkDestroyImageView    (vkDevice, vkColorImageView,         nullptr);
    vkDestroyImage        (vkDevice, vkColorImage,             nullptr);
    vkFreeMemory          (vkDevice, vkColorImageMemory,       nullptr);
    vkDestroySwapchainKHR (vkDevice, vkSwapChain,              nullptr);
}

void Renderer::DestroyDescriptorLayoutsAndPools() const
{
    gpuData->DestroyArray<Resources::Model>();
    gpuData->DestroyArray<Resources::Material>();
    gpuData->DestroyArray<Resources::Light>();

    if (constDataDescriptorLayout) vkDestroyDescriptorSetLayout(vkDevice, constDataDescriptorLayout, nullptr);
    if (constDataDescriptorPool)   vkDestroyDescriptorPool     (vkDevice, constDataDescriptorPool,   nullptr);
}
#pragma endregion 

#pragma region Rendering
void Renderer::NewFrame()
{
    // Wait for the previous frame to finish.
    vkWaitForFences(vkDevice, 1, &vkInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // Acquire an image from the swap chain.
    const VkResult result = vkAcquireNextImageKHR(vkDevice, vkSwapChain, UINT64_MAX, vkImageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &vkSwapChainImageIndex);

    // Recreate the swap chain if it is out of date.
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain();
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LogError(LogType::Vulkan, "Failed to acquire swap chain image.");
        throw std::runtime_error("VULKAN_IMAGE_ACQUISITION_ERROR");
    }

    // Only reset fences if we are going to be submitting work.
    vkResetFences(vkDevice, 1, &vkInFlightFences[currentFrame]);

    // Reset the command buffer.
    vkResetCommandBuffer(vkCommandBuffers[currentFrame],  0);
    
    // Begin recording the command buffer.
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(vkCommandBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to begin recording command buffer.");
        throw std::runtime_error("VULKAN_BEGIN_COMMAND_BUFFER_ERROR");
    }
}

void Renderer::BeginRenderPass() const
{
    // Define the clear color.
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color        = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    clearValues[1].depthStencil = { 1.0f, 0 };

    // Begin the render pass.
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = vkRenderPass;
    renderPassInfo.framebuffer       = vkSwapChainFramebuffers[vkSwapChainImageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = { vkSwapChainWidth, vkSwapChainHeight };
    renderPassInfo.clearValueCount   = (uint32_t)clearValues.size();
    renderPassInfo.pClearValues      = clearValues.data();
    vkCmdBeginRenderPass(vkCommandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind the graphics pipeline.
    vkCmdBindPipeline(vkCommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, vkGraphicsPipeline);

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

void Renderer::EndRenderPass() const
{
    // End the render pass.
    vkCmdEndRenderPass(vkCommandBuffers[currentFrame]);

    // Stop recording the command buffer.
    if (vkEndCommandBuffer(vkCommandBuffers[currentFrame]) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to record command buffer.");
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
        LogError(LogType::Vulkan, "Failed to submit draw command buffer.");
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
    presentInfo.pImageIndices      = &vkSwapChainImageIndex;
    presentInfo.pResults           = nullptr; // Optional.
    const VkResult result = vkQueuePresentKHR(vkPresentQueue, &presentInfo);

    // Recreate the swap chain if it is out of date or suboptimal.
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        RecreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to present swap chain image.");
        throw std::runtime_error("VULKAN_PRESENTATION_ERROR");
    }

    // Move to the next frame.
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
#pragma endregion
