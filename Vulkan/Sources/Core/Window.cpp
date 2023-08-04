#include "Core/Application.h"
#include "Core/Window.h"

#include <imgui.h>

#include "GLFW/glfw3.h"
#include <iostream>
using namespace Core;

InputKeys::InputKeys()
{
    exit        = GLFW_KEY_ESCAPE;
    moveRight   = GLFW_KEY_D;
    moveLeft    = GLFW_KEY_A;
    moveUp      = GLFW_KEY_E;
    moveDown    = GLFW_KEY_Q;
    moveForward = GLFW_KEY_W;
    moveBack    = GLFW_KEY_S;
}

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
        LogError(LogType::GLFW, "Unable to create a window.");
        return;
    }
    glfwMakeContextCurrent(glfwWindow);
    SetPos(params.posX, params.posY);
    glfwSwapInterval(params.vsync);

    // Set the minimum size of the window.
    glfwSetWindowSizeLimits(glfwWindow, 1, 1, GLFW_DONT_CARE, GLFW_DONT_CARE);

    // Set window user pointer and callbacks.
    glfwSetWindowUserPointer (glfwWindow, this);
    glfwSetFramebufferSizeCallback(glfwWindow, [](GLFWwindow* _glfwWindow, int _width, int _height)
    {
        Window* window = (Window*)glfwGetWindowUserPointer(_glfwWindow);
        window->params.width = _width; window->params.height = _height;
        
        while (window->params.width == 0 || window->params.height == 0) {
            glfwGetFramebufferSize(_glfwWindow, &window->params.width, &window->params.height);
            glfwWaitEvents();
        }

        const Application* app = Application::Get();
        if (app)
        {
            app->GetRenderer()->ResizeSwapChain();
            app->GetEngine()->ResizeCamera(window->params.width, window->params.height);
        }
    });
    glfwSetWindowPosCallback(glfwWindow, [](GLFWwindow* _glfwWindow, int _posX, int _posY)
    {
        Window* window = (Window*)glfwGetWindowUserPointer(_glfwWindow);
        window->params.posX = _posX; window->params.posY = _posY;
    });
    glfwSetDropCallback(glfwWindow, [](GLFWwindow* window, int count, const char** paths)
    {
        if (const Application* app = Application::Get()) {
            for (int i = 0; i < count; i++) {
                app->GetEngine()->LoadFile(paths[i]);
            }
        }
    });

    // Initialize the time points.
    prevTime = curTime = std::chrono::high_resolution_clock::now();
}

Window::~Window()
{
    glfwDestroyWindow(glfwWindow);
}


void Window::Update()
{
    // Update the delta time.
    curTime   = std::chrono::high_resolution_clock::now();
    deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(curTime - prevTime).count();
    prevTime  = curTime;
    
    // Make sure the current context is the window.
    if (glfwWindow != glfwGetCurrentContext())
        glfwMakeContextCurrent(glfwWindow);

    // Update glfw events and update inputs.
    glfwPollEvents();
    UpdateInputs();
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

void Window::UpdateInputs()
{
    // Close the window if the exit key was pressed.
    if (inputKeys.exit > 0 && glfwGetKey(glfwWindow, inputKeys.exit) == GLFW_PRESS)
        Close();

    // Disable inputs if ImGui wants to use them.
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse || io.WantCaptureKeyboard)
    {
        inputs = WindowInputs();
        return;
    }
    
    // Get movement inputs.
    inputs.dirMovement = {};
    if (inputKeys.moveRight   > 0 && glfwGetKey(glfwWindow, inputKeys.moveRight  )) inputs.dirMovement.x += 1;
    if (inputKeys.moveLeft    > 0 && glfwGetKey(glfwWindow, inputKeys.moveLeft   )) inputs.dirMovement.x -= 1;
    if (inputKeys.moveUp      > 0 && glfwGetKey(glfwWindow, inputKeys.moveUp     )) inputs.dirMovement.y -= 1;
    if (inputKeys.moveDown    > 0 && glfwGetKey(glfwWindow, inputKeys.moveDown   )) inputs.dirMovement.y += 1;
    if (inputKeys.moveForward > 0 && glfwGetKey(glfwWindow, inputKeys.moveForward)) inputs.dirMovement.z -= 1;
    if (inputKeys.moveBack    > 0 && glfwGetKey(glfwWindow, inputKeys.moveBack   )) inputs.dirMovement.z += 1;

    // Get mouse button inputs.
    inputs.mouseLeftClick   = glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT);
    inputs.mouseMiddleClick = glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_MIDDLE);
    inputs.mouseRightClick  = glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_RIGHT);

    // Get mouse position and mouse delta.
    double mouseX; double mouseY;
    glfwGetCursorPos(glfwWindow, &mouseX, &mouseY);
    const Maths::Vector2 prevMousePos = inputs.mousePos;
    const Maths::Vector2 mousePos     = { (float)mouseX, (float)mouseY };
    inputs.mouseDelta = mousePos - prevMousePos;
    inputs.mousePos   = mousePos;

    // Hide the mouse cursor and disable its movement when right clicking.
    if (inputs.mouseRightClick)
    {
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        glfwSetCursorPos(glfwWindow, (double)prevMousePos.x, (double)prevMousePos.y);
        inputs.mousePos = prevMousePos;
    }
    else
    {
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}
