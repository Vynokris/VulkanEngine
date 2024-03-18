#pragma once

namespace Core
{
    class  Application;
    class  Window;
    struct WindowParams;
    class  Logger;
    class  Renderer;
    class  GpuDataManager;
    class  Engine;
    class  UserInterface;
    
    class Application
    {
    private:
        inline static Application* instance = nullptr;
        
        Logger*         logger   = nullptr;
        Window*         window   = nullptr;
        GpuDataManager* gpuData  = nullptr;
        Renderer*       renderer = nullptr;
        Engine*         engine   = nullptr;
        UserInterface*  ui       = nullptr;

    private:
        Application() = default;
        
    public:
        Application(const Application&)            = delete;
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

        Logger*         GetLogger()   const { return logger;   }
        Window*         GetWindow()   const { return window;   }
        Renderer*       GetRenderer() const { return renderer; }
        GpuDataManager* GetGpuData()  const { return gpuData;  }
        Engine*         GetEngine()   const { return engine;   }
        UserInterface*  GetUi()       const { return ui;       }
    };
}
