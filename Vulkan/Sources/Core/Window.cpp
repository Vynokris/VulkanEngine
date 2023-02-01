#include "Core/Window.h"
#include "GLFW/glfw3.h"
#include <iostream>
using namespace Core;

Window::Window(const char* windowName, const int& windowWidth, const int& windowHeight, const int& windowPosX, const int& windowPosY, const bool& windowVsync)
    : name(windowName), width(windowWidth), height(windowHeight), posX(windowPosX), posY(windowPosY), vsync(windowVsync)
{
    // Initialize glfw.
    if (!glfwInit()) {
        std::cout << "ERROR (GLFW): Unable to initialize GLFW." <<std::endl;
        return;
    }

    // Set the window size to the monitor size if windowWidth and windowHeight are not positive.
    if (width < 0 && height < 0)
    {
              GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* vidMode = glfwGetVideoMode(monitor);
        width = vidMode->width; height = vidMode->height - 30; // TODO: check this on different monitor resolutions.
        posX = 0; posY = 30;
    }
    
    // Create a new glfw window with the given parameters.
    glfwWindow = glfwCreateWindow(width, height, name, NULL, NULL);
    if (!glfwWindow) {
        std::cout << "ERROR (GLFW): Unable to create a window." <<std::endl;
        return;
    }
    glfwMakeContextCurrent(glfwWindow);
    SetPos(posX, posY);
    glfwSwapInterval(vsync);
    exitKey = GLFW_KEY_ESCAPE;

    // Set window user pointer and callbacks.
    glfwSetWindowUserPointer (glfwWindow, this);
    glfwSetWindowSizeCallback(glfwWindow, [](GLFWwindow* _glfwWindow, int _width, int _height)
    {
        Window* window = (Window*)glfwGetWindowUserPointer(_glfwWindow);
        window->width = _width; window->height = _height;
    });
    glfwSetWindowPosCallback(glfwWindow, [](GLFWwindow* _glfwWindow, int _posX, int _posY)
    {
        Window* window = (Window*)glfwGetWindowUserPointer(_glfwWindow);
        window->posX = _posX; window->posY = _posY;
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
    if (exitKey > 0 && glfwGetKey(glfwWindow, exitKey) == GLFW_PRESS)
        glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
}

void Window::EndFrame() const
{
    glfwSwapBuffers(glfwWindow);
}

void Window::SetName(const char* windowName)
{
    name = windowName;
    glfwSetWindowTitle(glfwWindow, name);
}

void Window::SetWidth (const int& windowWidth)                          const { glfwSetWindowSize(glfwWindow, windowWidth, height); }
void Window::SetHeight(                        const int& windowHeight) const { glfwSetWindowSize(glfwWindow, width, windowHeight); }
void Window::SetSize  (const int& windowWidth, const int& windowHeight) const { glfwSetWindowSize(glfwWindow, windowWidth, windowHeight); }
void Window::SetPosX  (const int& windowPosX)                           const { glfwSetWindowPos(glfwWindow, windowPosX, posY); }
void Window::SetPosY  (                       const int& windowPosY)    const { glfwSetWindowPos(glfwWindow, posX, windowPosY); }
void Window::SetPos   (const int& windowPosX, const int& windowPosY)    const { glfwSetWindowPos(glfwWindow, windowPosX, windowPosY); }

void Window::SetVsync(const bool& windowVsync)
{
    if (glfwWindow != glfwGetCurrentContext())
        glfwMakeContextCurrent(glfwWindow);
    
    vsync = windowVsync;
    glfwSwapInterval(vsync);
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(glfwWindow);
}
