#include "Core/Application.h"
#include "Core/Window.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "Core/RendererShadows.h"
#include "Core/GpuDataManager.h"
#include "Core/Engine.h"
#include "Core/UserInterface.h"
#include <GLFW/glfw3.h>
#include <iostream>
using namespace Core;

Application* Application::Create()
{
    if (!instance)
        instance = new Application;
    return instance;
}

Application* Application::Get()
{
    return instance;
}

void Application::Destroy()
{
    delete instance;
}

void Application::Init(const WindowParams& windowParams)
{
    if (!glfwInit()) {
        LogError(LogType::GLFW, "Unable to initialize GLFW.");
        throw std::runtime_error("GLFW_INIT_ERROR");
    }
    
    logger   = new Logger("Resources/app.log");
    window   = new Window(windowParams);
    gpuData  = new GpuDataManager();
    renderer = new Renderer(this, windowParams.name, "No Engine", windowParams.vsync);
    engine   = new Engine(this);
    ui       = new UserInterface(this, engine);

    engine->Awake();
}

void Application::Run() const
{
    if (!instance || !window)
        return;
    
    engine->Start();
    while(!window->ShouldClose())
    {
        window->Update();
        engine->Update((float)window->GetDeltaTime());

        renderer->NewFrame();
        {
            // Render shadow maps for the first light in the scene.
            RendererShadows* rendererShadows = renderer->GetRendererShadows();
            const Resources::LightType lightType = engine->GetLight(0)->type;
            rendererShadows->BeginRender(lightType);
            {
                const uint32_t renderCount = lightType == Resources::LightType::Point ? 4 : 1;
                for (uint32_t renderIdx = 0; renderIdx < renderCount; renderIdx++)
                {
                    engine->Render(rendererShadows);
                    rendererShadows->NextRender(lightType);
                }
            }
            rendererShadows->EndRender();

            // Render the scene from the camera's point of view.
            renderer->BeginRenderPass();
            engine->Render(renderer);
            ui->Render();
        }
        renderer->EndRender();
        window->EndFrame();
    }
}

void Application::Quit() const
{
    window->Close();
}

void Application::Release() const
{
    renderer->WaitUntilIdle();
    delete ui;
    delete engine;
    delete renderer;
    delete logger;
    delete window;
    glfwTerminate();
}
