#pragma once
#include "Window.h"
#include "Renderer.h"
#include "Engine.h"
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
        Engine*        engine   = nullptr;
        UserInterface* ui       = nullptr;
        
    public:
        static Application* Create();
        static Application* Get();
        static void         Destroy();

        void Init(const WindowParams& windowParams);
        void Run() const;
        void Quit() const;
        void Release() const;

        Window*        GetWindow()   const { return window; }
        Renderer*      GetRenderer() const { return renderer; }
        Engine*        GetEngine()   const { return engine; }
        UserInterface* GetUi()       const { return ui; }
    };
}
