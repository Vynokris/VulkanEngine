#pragma once
#include "GraphicsUtils.h"

namespace Resources { class Camera; class Model; class Light; enum class LightType : int; }

namespace Core
{
    class Application;
    class Renderer;
    class GpuDataManager;

    class RendererShadows
    {
    private:
        Application*          app;
        Renderer*             renderer;
        GpuDataManager*       gpuData;
        VkRenderPass          vkRenderPass          = nullptr;
        VkSampler             vkShadowSampler       = nullptr;
        VkImage               vkShadowImage         = nullptr;
        VkDeviceMemory        vkShadowImageMemory   = nullptr;
        VkImageView           vkShadowImageView     = nullptr;
        VkFramebuffer         vkShadowFramebuffer   = nullptr;
        VkDescriptorSetLayout vkDescriptorSetLayout = nullptr;
        VkDescriptorPool      vkDescriptorPool      = nullptr;
        VkDescriptorSet       vkDescriptorSet       = nullptr;
        VkBuffer              vkBuffer              = nullptr;
        VkDeviceMemory        vkBufferMemory        = nullptr;
        void*                 vkBufferMapped        = nullptr;
        VkPipelineLayout      vkPipelineLayout      = nullptr;
        VkPipeline            vkPipeline            = nullptr;
        VkFormat              vkShadowImageFormat   = (VkFormat)0;
        VkDeviceSize          vkBufferSize          = 0;
        uint32_t              vkShadowImageWidth    = 0, vkShadowImageHeight = 0;
        uint32_t              renderIdx             = 0;
        bool                  framebufferResized    = false;
        // VkCommandPool                     vkCommandPool;
        // VkCommandBuffer                   vkCommandBuffer;
        // VkSemaphore                       vkImageAvailableSemaphore;
        // VkSemaphore                       vkRenderFinishedSemaphores;
        // VkFence                           vkInFlightFence;
        // VkImage                           vkSwapChainImage;
        // VkImageView                       vkSwapChainImageView;
        // VkFormat                          vkSwapChainImageFormat;
        
    public:
        RendererShadows(Application* application, Renderer* appRenderer, uint32_t shadowMapWidth, uint32_t shadowMapHeight);
        RendererShadows(const RendererShadows&)            = delete;
        RendererShadows(RendererShadows&&)                 = delete;
        RendererShadows& operator=(const RendererShadows&) = delete;
        RendererShadows& operator=(RendererShadows&&)      = delete;
        ~RendererShadows();

        void BeginRender(Resources::LightType lightType);
        void NextRender (Resources::LightType lightType);
        void DrawModel(const Resources::Model& model) const;
        void EndRender();

        void WaitUntilIdle() const;
        void ResizeSwapChain() { framebufferResized = true; }

        VkRenderPass          GetVkRenderPass()          const { return vkRenderPass;          }
        VkSampler             GetVkSampler()             const { return vkShadowSampler;       }
        uint32_t              GetVkShadowImageWidth()    const { return vkShadowImageWidth;    }
        uint32_t              GetVkShadowImageHeight()   const { return vkShadowImageHeight;   }
        void*                 GetVkBufferMapped()        const { return vkBufferMapped;        }
        VkDeviceSize          GetVkBufferSize()          const { return vkBufferSize;          }
        VkDescriptorSetLayout GetVkDescriptorSetLayout() const { return vkDescriptorSetLayout; }
        VkDescriptorSet       GetVkDescriptorSet()       const { return vkDescriptorSet;       }
        VkPipelineLayout      GetVkPipelineLayout()      const { return vkPipelineLayout;      }
        // VkCommandPool         GetVkCommandPool()         const { return vkCommandPool; }
        // VkCommandBuffer       GetVkCommandBuffer()       const { return vkCommandBuffer; }

    private:
        void CreateRenderPass();
        void CreateTextureSampler();
        void CreateShadowImage();
        void CreateDescriptors();
        void CreatePipeline();
        // void CreateCommandPool();
        // void CreateCommandBuffers();
        // void CreateSyncObjects();

        void RecreateShadowImage();
        void DestroyShadowImage() const;

        void NewFrame();
        void BeginRenderPass() const;
        void UpdateViewportScissor(Resources::LightType lightType) const;
        void EndRenderPass() const;
    };
}
