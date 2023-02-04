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
        std::cout << "ERROR (GLFW): Unable to initialize GLFW." <<std::endl;
        throw std::runtime_error("GLFW_INIT_ERROR");
    }
    window   = new Window(windowParams);
    renderer = new Renderer(windowParams.name);
    ui       = new UserInterface();
}

void Application::Run()
{
    if (!instance || !window)
        return;

    while(!window->ShouldClose())
    {
        window->Update();
        Update();
        
        renderer->BeginRender();
        {
            Render();
            ui->Render();
        }
        renderer->EndRender();
        window->EndFrame();
    }
}

void Application::Quit()
{
    window->Close();
}

void Application::Release()
{
    delete ui;
    delete renderer;
    delete window;
    glfwTerminate();
}

void Application::Update()
{
    
}

void Application::Render()
{
    renderer->DrawFrame();
}
