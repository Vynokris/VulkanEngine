#pragma once
#include <cstdint>
#include <optional>
#include <vector>

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

#pragma region Forward Declarations
// Forward declaration of Vulkan types to avoid include inside header.
typedef struct VkInstance_T*            VkInstance;
typedef struct VkPhysicalDevice_T*      VkPhysicalDevice;
typedef struct VkDevice_T*              VkDevice;
typedef struct VkQueue_T*               VkQueue;
typedef struct VkSurfaceKHR_T*          VkSurfaceKHR;
typedef struct VkSwapchainKHR_T*        VkSwapchainKHR;
typedef struct VkFramebuffer_T*         VkFramebuffer;
typedef struct VkImage_T*               VkImage;
typedef struct VkImageView_T*           VkImageView;
typedef struct VkShaderModule_T*        VkShaderModule;
typedef struct VkRenderPass_T*          VkRenderPass;
typedef struct VkPipelineLayout_T*      VkPipelineLayout;
typedef struct VkPipeline_T*            VkPipeline;
typedef struct VkCommandPool_T*         VkCommandPool;
typedef struct VkCommandBuffer_T*       VkCommandBuffer;
typedef struct VkSemaphore_T*           VkSemaphore;
typedef struct VkFence_T*               VkFence;
typedef struct VkSurfaceCapabilitiesKHR VkSurfaceCapabilitiesKHR;
typedef struct VkSurfaceFormatKHR       VkSurfaceFormatKHR;
typedef struct VkExtent2D               VkExtent2D;
typedef enum   VkPresentModeKHR : int   VkPresentModeKHR;
typedef enum   VkFormat         : int   VkFormat;
#pragma endregion 

#pragma region VulkanUtils
namespace VulkanUtils
{
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        
        bool IsComplete() const
        {
            return graphicsFamily.has_value()
                && presentFamily .has_value();
        }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR*       capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;

        SwapChainSupportDetails();
        SwapChainSupportDetails(const SwapChainSupportDetails& other);
        ~SwapChainSupportDetails();

        bool IsAdequate() const
        {
            return !formats.empty() && !presentModes.empty();
        }
    };
    
    QueueFamilyIndices      FindQueueFamilies          (const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
    SwapChainSupportDetails QuerySwapChainSupport      (const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
    VkSurfaceFormatKHR      ChooseSwapSurfaceFormat    (const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR        ChooseSwapPresentMode      (const std::vector<VkPresentModeKHR  >& availablePresentModes);
    VkExtent2D              ChooseSwapExtent           (const VkSurfaceCapabilitiesKHR& capabilities);
    bool                    CheckDeviceExtensionSupport(const VkPhysicalDevice& device);
    bool                    IsDeviceSuitable           (const VkPhysicalDevice& device, const VkSurfaceKHR& surface);

    VkShaderModule CreateShaderModule(const VkDevice& device, const char* filename);
}
#pragma endregion


namespace Core
{
    class Application;

    class Renderer
    {
    private:
        Application*                 app;
        VkInstance                   vkInstance                = nullptr;
        VkSurfaceKHR                 vkSurface                 = nullptr;
        VkPhysicalDevice             vkPhysicalDevice          = nullptr;
        VkDevice                     vkDevice                  = nullptr;
        VkQueue                      vkGraphicsQueue           = nullptr;
        VkQueue                      vkPresentQueue            = nullptr;
        VkSwapchainKHR               vkSwapChain               = nullptr;
        VkRenderPass                 vkRenderPass              = nullptr;
        VkPipelineLayout             vkPipelineLayout          = nullptr;
        VkPipeline                   vkGraphicsPipeline        = nullptr;
        VkCommandPool                vkCommandPool             = nullptr;
        std::vector<VkCommandBuffer> vkCommandBuffers;
        std::vector<VkSemaphore>     vkImageAvailableSemaphores;
        std::vector<VkSemaphore>     vkRenderFinishedSemaphores;
        std::vector<VkFence>         vkInFlightFences;
        std::vector<VkFramebuffer>   vkSwapChainFramebuffers;
        std::vector<VkImage>         vkSwapChainImages;
        std::vector<VkImageView>     vkSwapChainImageViews;
        VkFormat                     vkSwapChainImageFormat;
        uint32_t                     vkSwapChainWidth = 0, vkSwapChainHeight = 0;
        uint32_t                     vkSwapchainImageIndex = 0;
        uint32_t                     currentFrame = 0;
        bool                         framebufferResized = false;
        
    public:
        Renderer(const char* appName, const char* engineName = "No Engine");
        ~Renderer();

        void BeginRender();
        void DrawFrame() const;
        void EndRender();

        void ResizeSwapChain() { framebufferResized = true; }

    private:
        void CheckValidationLayers() const;
        void CreateVkInstance(const char* appName, const char* engineName);
        void CreateSurface();
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateSwapChain();
        void CreateImageViews();
        void CreateRenderPass();
        void CreateGraphicsPipeline();
        void CreateFramebuffers();
        void CreateCommandPool();
        void CreateCommandBuffers();
        void CreateSyncObjects();

        void DestroySwapChain() const;
        void RecreateSwapChain();

        void NewFrame();
        void BeginRecordCmdBuf() const;
        void EndRecordCmdBuf() const;
        void PresentFrame();
    };
}
