#include "Core/Application.h"
#include "Core/Window.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
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
    renderer = new Renderer(this, windowParams.name);
    gpuData  = new GpuDataManager(renderer);
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
        
        renderer->BeginRender();
        {
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
