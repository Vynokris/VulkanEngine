#include "Core/Application.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
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
        std::cout << "ERROR (GLFW): Unable to initialize GLFW." <<std::endl;
        throw std::runtime_error("GLFW_INIT_ERROR");
    }
    window   = new Window(windowParams);
    renderer = new Renderer(windowParams.name);
    ui       = new UserInterface();
    engine   = new Engine();

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
    delete window;
    glfwTerminate();
}
