#include "Core/Window.h"
#include <GLFW/glfw3.h>
#include <iostream>

int main()
{	
	Core::Window* window = new Core::Window("Vulkan");
	while (!window->ShouldClose())
	{
		window->Update();
		window->EndFrame();
	}
	
	return 0;
}
