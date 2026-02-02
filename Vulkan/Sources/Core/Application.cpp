#include "Core/Application.h"
#include "Core/Window.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "Core/GpuDataManager.h"
#include "Core/Engine.h"
#include "Core/UserInterface.h"
#include <GLFW/glfw3.h>
#include <iostream>

#include "Core/RendererEnvmap.h"
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
    renderer = new Renderer(this, windowParams.name);
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
        engine->Update(window->GetDeltaTime());
        
        renderer->NewFrame();
        
        renderer->BeginRenderPass();
        engine->Render(renderer);
        renderer->EndRenderPass();

        RendererEnvmap* rendererEnvmap = renderer->GetRendererEnvmap();
        rendererEnvmap->BeginRenderPass();
        rendererEnvmap->Render();
        ui->Render();
        rendererEnvmap->EndRenderPass();
        
        renderer->PresentFrame();
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
