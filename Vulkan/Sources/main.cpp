#include "Core/Application.h"
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
