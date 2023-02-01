#include "Core/UserInterface.h"
#include "Core/Application.h"
using namespace Core;

UserInterface::UserInterface()
{
    app = Application::Get();
    // TODO: initialize imgui.
}

UserInterface::~UserInterface()
{
    // TODO: unload imgui.
}

void UserInterface::Render()
{
    NewFrame();
    EndFrame();
}

void UserInterface::NewFrame()
{
    // TODO: start a new imgui frame.
}

void UserInterface::EndFrame()
{
    // TODO: end the current imgui frame.
}
