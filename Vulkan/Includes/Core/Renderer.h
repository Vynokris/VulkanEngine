﻿#pragma once
#include "VulkanUtils.h"

namespace Resources
{
    class Camera;
    class Model;
}

namespace Core
{
    class Application;

    class Renderer
    {
    private:
        Application*                    app;
        VkInstance                      vkInstance            = nullptr;
        VkDebugUtilsMessengerEXT        vkDebugMessenger     = nullptr;
        VkSurfaceKHR                    vkSurface             = nullptr;
        VkPhysicalDevice                vkPhysicalDevice      = nullptr;
        VkDevice                        vkDevice              = nullptr;
        VulkanUtils::QueueFamilyIndices vkQueueFamilyIndices;
        VkQueue                         vkGraphicsQueue       = nullptr;
        VkQueue                         vkPresentQueue        = nullptr;
        VkSwapchainKHR                  vkSwapChain           = nullptr;
        VkRenderPass                    vkRenderPass          = nullptr;
        VkPipelineLayout                vkPipelineLayout      = nullptr;
        VkPipeline                      vkGraphicsPipeline    = nullptr;
        VkCommandPool                   vkCommandPool         = nullptr;
        VkSampler                       vkTextureSampler      = nullptr;
        VkImage                         vkColorImage          = nullptr;
        VkDeviceMemory                  vkColorImageMemory    = nullptr;
        VkImageView                     vkColorImageView      = nullptr;
        VkImage                         vkDepthImage          = nullptr;
        VkDeviceMemory                  vkDepthImageMemory    = nullptr;
        VkImageView                     vkDepthImageView      = nullptr;
        VkFormat                        vkDepthImageFormat;
        std::vector<VkCommandBuffer>    vkCommandBuffers;
        std::vector<VkSemaphore>        vkImageAvailableSemaphores;
        std::vector<VkSemaphore>        vkRenderFinishedSemaphores;
        std::vector<VkFence>            vkInFlightFences;
        std::vector<VkFramebuffer>      vkSwapChainFramebuffers;
        std::vector<VkImage>            vkSwapChainImages;
        std::vector<VkImageView>        vkSwapChainImageViews;
        VkFormat                        vkSwapChainImageFormat;
        uint32_t                        vkSwapChainWidth = 0, vkSwapChainHeight = 0;
        uint32_t                        vkSwapchainImageIndex = 0;
        VkSampleCountFlagBits           msaaSamples;
        uint32_t                        currentFrame = 0;
        bool                            framebufferResized = false;
        
    public:
        Renderer(const char* appName, const char* engineName = "No Engine");
        Renderer(const Renderer&)            = delete;
        Renderer(Renderer&&)                 = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&)      = delete;
        ~Renderer();

        void BeginRender();
        void DrawModel(const Resources::Model& model, const Resources::Camera* camera) const;
        void EndRender();

        void WaitUntilIdle() const;
        void ResizeSwapChain() { framebufferResized = true; }

        VkInstance            GetVkInstance()            const { return vkInstance;                                  }
        VkPhysicalDevice      GetVkPhysicalDevice()      const { return vkPhysicalDevice;                            }
        VkDevice              GetVkDevice()              const { return vkDevice;                                    }
        uint32_t              GetVkGraphicsQueueIndex()  const { return vkQueueFamilyIndices.graphicsFamily.value(); }
        VkQueue               GetVkGraphicsQueue()       const { return vkGraphicsQueue;                             }
        uint32_t              GetVkSwapchainImageCount() const { return (uint32_t)vkSwapChainImages.size();          }
        VkRenderPass          GetVkRenderPass()          const { return vkRenderPass;                                }
        VkCommandPool         GetVkCommandPool()         const { return vkCommandPool;                               }
        VkCommandBuffer       GetCurVkCommandBuffer()    const { return vkCommandBuffers[currentFrame];              }
        VkSampler             GetVkTextureSampler()      const { return vkTextureSampler;                            }
        VkSampleCountFlagBits GetMsaaSamples()           const { return msaaSamples;                                 }

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
        void CreateDescriptorLayoutsAndPools() const;
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

        void NewFrame();
        void BeginRenderPass() const;
        void EndRenderPass() const;
        void PresentFrame();
    };
}
