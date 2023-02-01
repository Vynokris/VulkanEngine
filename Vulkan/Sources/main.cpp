#include "Core/Application.h"
#include <GLFW/glfw3.h>
#include <iostream>
using namespace Core;

int main()
{	
	Application* app = Application::Create();
	app->Init({ "Vulkan" });
	app->Run();
	app->Release();
	Application::Destroy();
	
	return 0;
}
