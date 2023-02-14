#pragma once
#include "VulkanUtils.h"
#include "Maths/Matrix.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 3;

// TODO: Try to enable alpha blending.

namespace Resources
{
    class Camera;
    class Model;
}

// TODO: Temporary.
struct UniformBufferObject {
    Maths::Mat4 model;
    Maths::Mat4 view;
    Maths::Mat4 proj;
};
// End temporary.

namespace Core
{
    class Application;

    class Renderer
    {
    private:
        Application*                 app;
        VkInstance                   vkInstance            = nullptr;
        VkSurfaceKHR                 vkSurface             = nullptr;
        VkPhysicalDevice             vkPhysicalDevice      = nullptr;
        VkDevice                     vkDevice              = nullptr;
        VkQueue                      vkGraphicsQueue       = nullptr;
        VkQueue                      vkPresentQueue        = nullptr;
        VkSwapchainKHR               vkSwapChain           = nullptr;
        VkRenderPass                 vkRenderPass          = nullptr;
        VkDescriptorSetLayout        vkDescriptorSetLayout = nullptr;
        VkPipelineLayout             vkPipelineLayout      = nullptr;
        VkPipeline                   vkGraphicsPipeline    = nullptr;
        VkCommandPool                vkCommandPool         = nullptr;
        VkSampler                    vkTextureSampler      = nullptr;
        VkImage                      vkColorImage          = nullptr;
        VkDeviceMemory               vkColorImageMemory    = nullptr;
        VkImageView                  vkColorImageView      = nullptr;
        VkImage                      vkDepthImage          = nullptr;
        VkDeviceMemory               vkDepthImageMemory    = nullptr;
        VkImageView                  vkDepthImageView      = nullptr;
        VkFormat                     vkDepthImageFormat;
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
        VkSampleCountFlagBits        msaaSamples;
        uint32_t                     currentFrame = 0;
        bool                         framebufferResized = false;
        
    public:
        Renderer(const char* appName, const char* engineName = "No Engine");
        ~Renderer();

        void BeginRender();
        void DrawModel(const Resources::Model* model, const Resources::Camera* camera) const;
        void EndRender();

        void WaitUntilIdle() const;
        void ResizeSwapChain() { framebufferResized = true; }

        VkInstance            GetVkInstance()            const { return vkInstance;            }
        VkPhysicalDevice      GetVkPhysicalDevice()      const { return vkPhysicalDevice;      }
        VkDevice              GetVkDevice()              const { return vkDevice;              }
        VkQueue               GetVkGraphicsQueue()       const { return vkGraphicsQueue;       }
        VkCommandPool         GetVkCommandPool()         const { return vkCommandPool;         }
        VkSampler             GetVkTextureSampler()      const { return vkTextureSampler;      }
        VkDescriptorSetLayout GetVkDescriptorSetLayout() const { return vkDescriptorSetLayout; }
        uint32_t              GetCurrentFrame()          const { return currentFrame;          }

    private:
        void CheckValidationLayers() const;
        void CreateVkInstance(const char* appName, const char* engineName);
        void CreateSurface();
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateSwapChain();
        void CreateImageViews();
        void CreateRenderPass();
        void CreateDescriptorSetLayout();
        void CreateGraphicsPipeline();
        void CreateColorResources();
        void CreateDepthResources();
        void CreateFramebuffers();
        void CreateCommandPool();
        void CreateTextureSampler();
        void CreateCommandBuffers();
        void CreateSyncObjects();

        void DestroySwapChain() const;
        void RecreateSwapChain();

        void BindMeshBuffers(const VkBuffer& vertexBuffer, const VkBuffer& indexBuffer) const;

        void NewFrame();
        void BeginRecordCmdBuf() const;
        void EndRecordCmdBuf() const;
        void PresentFrame();
    };
}
