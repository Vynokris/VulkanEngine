#pragma once
#include "Window.h"
#include "Renderer.h"
#include "UserInterface.h"

namespace Core
{
    class Application;
    class Application
    {
    private:
        inline static Application* instance = nullptr;
        
        Window*        window   = nullptr;
        Renderer*      renderer = nullptr;
        UserInterface* ui       = nullptr;
        
    public:
        static Application* Create();
        static Application* Get();
        static void         Destroy();

        void Init(const WindowParams& windowParams);
        void Run();
        void Quit();
        void Release();

        Window*        GetWindow()   const { return window; }
        Renderer*      GetRenderer() const { return renderer; }
        UserInterface* GetUi()       const { return ui; }

    private:
        void Update();
        void Render();
    };
}
