#include "Core/Application.h"
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
    window = new Window(windowParams);
}

void Application::Run()
{
    if (!instance || !window)
        return;

    while(!window->ShouldClose())
    {
        window->Update();
        Update();
        Render();
        window->EndFrame();
    }
}

void Application::Quit()
{
    window->Close();
}

void Application::Release()
{
    delete window;
}

void Application::Update()
{
    
}

void Application::Render()
{
    
}
