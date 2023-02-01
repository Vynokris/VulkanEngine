#pragma once

namespace Core
{
    class Application;
    
    class UserInterface
    {
    private:
        Application* app;
        
    public:
        UserInterface();
        ~UserInterface();

        void Render();

    private:
        void NewFrame();
        void EndFrame();
    };
}