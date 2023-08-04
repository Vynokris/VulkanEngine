#pragma once
#include "Window.h"
#include "Logger.h"
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
        Logger*        logger   = nullptr;
        Renderer*      renderer = nullptr;
        Engine*        engine   = nullptr;
        UserInterface* ui       = nullptr;

    private:
        Application() = default;
        
    public:
        Application(const Application& other)      = delete;
        Application(Application&&)                 = delete;
        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&)      = delete;
        ~Application()                             = default;
        
        static Application* Create();
        static Application* Get();
        static void         Destroy();

        void Init(const WindowParams& windowParams);
        void Run() const;
        void Quit() const;
        void Release() const;

        Window*        GetWindow()   const { return window;   }
        Logger*        GetLogger()   const { return logger;   }
        Renderer*      GetRenderer() const { return renderer; }
        Engine*        GetEngine()   const { return engine;   }
        UserInterface* GetUi()       const { return ui;       }
    };
}
