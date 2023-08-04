#include "Core/Application.h"
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
    window   = new Window(windowParams);
    logger   = new Logger("Resources/app.log");
    renderer = new Renderer(windowParams.name);
    engine   = new Engine();
    ui       = new UserInterface();

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
