#include "Core/Window.h"
#include "GLFW/glfw3.h"
#include <iostream>
using namespace Core;

Window::Window(const WindowParams& windowParams)
    : params(windowParams)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Set the window size to the monitor size if windowWidth and windowHeight are not positive.
    if (params.width < 0 && params.height < 0)
    {
              GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* vidMode = glfwGetVideoMode(monitor);
        params.width = vidMode->width; params.height = vidMode->height - 30; // TODO: check this on different monitor resolutions.
        params.posX = 0; params.posY = 30;
    }
    
    // Create a new glfw window with the given parameters.
    glfwWindow = glfwCreateWindow(params.width, params.height, params.name, NULL, NULL);
    if (!glfwWindow) {
        std::cout << "ERROR (GLFW): Unable to create a window." <<std::endl;
        return;
    }
    glfwMakeContextCurrent(glfwWindow);
    SetPos(params.posX, params.posY);
    glfwSwapInterval(params.vsync);
    params.exitKey = GLFW_KEY_ESCAPE;

    // Set window user pointer and callbacks.
    glfwSetWindowUserPointer (glfwWindow, this);
    glfwSetWindowSizeCallback(glfwWindow, [](GLFWwindow* _glfwWindow, int _width, int _height)
    {
        Window* window = (Window*)glfwGetWindowUserPointer(_glfwWindow);
        window->params.width = _width; window->params.height = _height;
    });
    glfwSetWindowPosCallback(glfwWindow, [](GLFWwindow* _glfwWindow, int _posX, int _posY)
    {
        Window* window = (Window*)glfwGetWindowUserPointer(_glfwWindow);
        window->params.posX = _posX; window->params.posY = _posY;
    });
}

Window::~Window()
{
    glfwDestroyWindow(glfwWindow);
}


void Window::Update() const
{
    // Make sure the current context is the window.
    if (glfwWindow != glfwGetCurrentContext())
        glfwMakeContextCurrent(glfwWindow);

    // Update glfw events and kill the window if the exit key is pressed.
    glfwPollEvents();
    if (params.exitKey > 0 && glfwGetKey(glfwWindow, params.exitKey) == GLFW_PRESS)
        Close();
}

void Window::EndFrame() const
{
    glfwSwapBuffers(glfwWindow);
}

void Window::Close() const
{
    glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
}


void Window::SetName(const char* windowName)
{
    params.name = windowName;
    glfwSetWindowTitle(glfwWindow, params.name);
}

void Window::SetWidth (const int& windowWidth)                          const { glfwSetWindowSize(glfwWindow, windowWidth, params.height); }
void Window::SetHeight(                        const int& windowHeight) const { glfwSetWindowSize(glfwWindow, params.width, windowHeight); }
void Window::SetSize  (const int& windowWidth, const int& windowHeight) const { glfwSetWindowSize(glfwWindow, windowWidth, windowHeight); }
void Window::SetPosX  (const int& windowPosX)                           const { glfwSetWindowPos(glfwWindow, windowPosX, params.posY); }
void Window::SetPosY  (                       const int& windowPosY)    const { glfwSetWindowPos(glfwWindow, params.posX, windowPosY); }
void Window::SetPos   (const int& windowPosX, const int& windowPosY)    const { glfwSetWindowPos(glfwWindow, windowPosX, windowPosY); }

void Window::SetVsync(const bool& windowVsync)
{
    if (glfwWindow != glfwGetCurrentContext())
        glfwMakeContextCurrent(glfwWindow);
    
    params.vsync = windowVsync;
    glfwSwapInterval(params.vsync);
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(glfwWindow);
}
