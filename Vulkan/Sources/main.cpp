#include "Core/Application.h"
using namespace Core;

int main()
{
	std::cout << sizeof(VulkanUtils::MaterialData) << std::endl;
	
	Application* app = Application::Create();
	app->Init({ "Vulkan" });
	app->Run();
	app->Release();
	Application::Destroy();
	
	return 0;
}
