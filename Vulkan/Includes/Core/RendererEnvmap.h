#pragma once
#include "GraphicsUtils.h"

namespace Resources { class Cubemap; }

namespace Core
{
    class Application;
    class Renderer;
    class GpuDataManager;

    class RendererEnvmap
    {
    private:
        Application*     app;
        Renderer*        renderer;
        GpuDataManager*  gpuData;
        VkRenderPass     vkRenderPass     = nullptr;
        VkPipelineLayout vkPipelineLayout = nullptr;
        VkPipeline       vkPipeline       = nullptr;

    public:
        RendererEnvmap(Application* application, Renderer* appRenderer);
        RendererEnvmap(const RendererEnvmap&)            = delete;
        RendererEnvmap(RendererEnvmap&&)                 = delete;
        RendererEnvmap& operator=(const RendererEnvmap&) = delete;
        RendererEnvmap& operator=(RendererEnvmap&&)      = delete;
        ~RendererEnvmap();

        void BeginRenderPass() const;
        void Render() const;
        void EndRenderPass() const;

    private:
        void CreateRenderPass();
        void CreatePipeline();
    };
}
