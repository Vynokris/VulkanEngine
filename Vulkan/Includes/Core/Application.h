#pragma once
#include "Window.h"

namespace Core
{
    class Application;
    class Application
    {
    private:
        inline static Application* instance = nullptr;
        
        Window* window = nullptr;
        
    public:
        static Application* Create();
        static Application* Get();
        static void         Destroy();

        void Init(const WindowParams& windowParams);
        void Run();
        void Quit();
        void Release();

    private:
        void Update();
        void Render();
    };
}
