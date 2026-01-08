#pragma once
#include "GpuDataManager.h"
#include "GraphicsUtils.h"

namespace Resources { class Camera; class Model; class Mesh; }
namespace Core
{
    class Application;
    class GpuDataManager;

    class Renderer
    {
    private:
        Application*                      app;
        GpuDataManager*                   gpuData;
        VkInstance                        vkInstance       = nullptr;
        VkDebugUtilsMessengerEXT          vkDebugMessenger = nullptr;
        VkSurfaceKHR                      vkSurface        = nullptr;
        VkPhysicalDevice                  vkPhysicalDevice = nullptr;
        VkDevice                          vkDevice         = nullptr;
        GraphicsUtils::QueueFamilyIndices vkQueueFamilyIndices;
        VkQueue                           vkGraphicsQueue          = nullptr;
        VkQueue                           vkPresentQueue           = nullptr;
        VkSwapchainKHR                    vkSwapChain              = nullptr;
        VkRenderPass                      vkRenderPass             = nullptr;
        VkPipelineLayout                  vkComputePipelineLayout  = nullptr;
        VkPipeline                        vkComputePipelines[5]    = { nullptr };
        VkPipelineLayout                  vkGraphicsPipelineLayout = nullptr;
        VkPipeline                        vkGraphicsPipeline       = nullptr;
        VkCommandPool                     vkCommandPool            = nullptr;
        VkSampler                         vkTextureSampler         = nullptr;
        VkImage                           vkColorImage             = nullptr;
        VkDeviceMemory                    vkColorImageMemory       = nullptr;
        VkImageView                       vkColorImageView         = nullptr;
        VkImage                           vkDepthImage             = nullptr;
        VkDeviceMemory                    vkDepthImageMemory       = nullptr;
        VkImageView                       vkDepthImageView         = nullptr;
        VkFormat                          vkDepthImageFormat;
        std::vector<VkCommandBuffer>      vkCommandBuffers;
        std::vector<VkSemaphore>          vkImageAvailableSemaphores;
        std::vector<VkSemaphore>          vkRenderFinishedSemaphores;
        std::vector<VkFence>              vkInFlightFences;
        std::vector<VkFramebuffer>        vkSwapChainFramebuffers;
        std::vector<VkImage>              vkSwapChainImages;
        std::vector<VkImageView>          vkSwapChainImageViews;
        VkFormat                          vkSwapChainImageFormat;
        uint32_t                          vkSwapChainWidth = 0, vkSwapChainHeight = 0;
        uint32_t                          vkSwapChainImageIndex = 0;
        VkSampleCountFlagBits             msaaSamples;
        VkDescriptorSetLayout             constDataDescriptorLayout = nullptr;
        VkDescriptorPool                  constDataDescriptorPool   = nullptr;
        VkDescriptorSet                   constDataDescriptorSet    = nullptr;
        VkBuffer                          fogParamsBuffer           = nullptr;
        VkDeviceMemory                    fogParamsBufferMemory     = nullptr;
        bool                              framebufferResized        = false;
        uint32_t                          currentFrame              = 0;
        
    public:
        Renderer(Application* application, const char* appName, const char* engineName = "No Engine");
        Renderer(const Renderer&)            = delete;
        Renderer(Renderer&&)                 = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&)      = delete;
        ~Renderer();

        void SetShaderFrameConstants(const GraphicsUtils::ShaderFrameConstants& constants) const;
        void SetDistanceFogParams(const Maths::RGB& color, const float& start, const float& end);

        void BeginRender();
        void DispatchIndirectCompute(const Resources::Model& model) const;
        void BeginRenderPass() const;
        void DrawModel(const Resources::Model& model) const;
        void EndRenderPass() const;
        void EndRender();

        void WaitUntilIdle() const;
        void ResizeSwapChain() { framebufferResized = true; }

        VkInstance            GetVkInstance()            const { return vkInstance; }
        VkSurfaceKHR          GetVkSurface()             const { return vkSurface; }
        VkPhysicalDevice      GetVkPhysicalDevice()      const { return vkPhysicalDevice; }
        VkDevice              GetVkDevice()              const { return vkDevice; }
        uint32_t              GetVkGraphicsQueueIndex()  const { return vkQueueFamilyIndices.graphicsAndComputeFamily.value(); }
        VkQueue               GetVkGraphicsQueue()       const { return vkGraphicsQueue; }
        uint32_t              GetVkSwapChainImageCount() const { return (uint32_t)vkSwapChainImages.size(); }
        VkFormat              GetVkDepthImageFormat()    const { return vkDepthImageFormat; }
        VkRenderPass          GetVkRenderPass()          const { return vkRenderPass; }
        VkPipelineLayout      GetVkPipelineLayout()      const { return vkGraphicsPipelineLayout; }
        VkCommandPool         GetVkCommandPool()         const { return vkCommandPool; }
        VkCommandBuffer       GetCurVkCommandBuffer()    const { return vkCommandBuffers[currentFrame]; }
        VkSampler             GetVkTextureSampler()      const { return vkTextureSampler; }
        VkSampleCountFlagBits GetMsaaSamples()           const { return msaaSamples; }

    private:
        void CheckValidationLayers() const;
        void CreateVkInstance(const char* appName, const char* engineName);
        void CreateDebugMessenger();
        void CreateSurface();
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateSwapChain();
        void CreateImageViews();
        void CreateRenderPass();
        void CreateDescriptorLayoutsAndPools();
        void CreateComputePipeline();
        void CreateGraphicsPipeline();
        void CreateColorResources();
        void CreateDepthResources();
        void CreateFramebuffers();
        void CreateCommandPool();
        void CreateTextureSampler();
        void CreateCommandBuffers();
        void CreateSyncObjects();

        void RecreateSwapChain();
        void DestroySwapChain() const;
        void DestroyDescriptorLayoutsAndPools() const;

        void NewFrame();
        void BeginCommandBuffer() const;
        void EndCommandBuffer() const;
        void PresentFrame();
    };
}
