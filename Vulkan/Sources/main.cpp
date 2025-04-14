#include "Core/Application.h"
#include "Core/Window.h"
using namespace Core;

int main()
{
	Application* app = Application::Create();
	WindowParams windowParams{};
	windowParams.name      = "Vulkan";
	windowParams.vsync     = true;
	windowParams.targetFPS = -1;
	app->Init(windowParams);
	app->Run();
	app->Release();
	Application::Destroy();
	
	return 0;
}
