#pragma once
typedef struct GLFWwindow GLFWwindow;

namespace Core
{
    struct WindowParams
    {
        const char* name = "";
        int  width  = -1;
        int  height = -1;
        int  posX = 50;
        int  posY = 70;
        bool vsync = false;
        int  exitKey = -1;
    };
    
    class Window
    {
    private:
        WindowParams params;
        GLFWwindow* glfwWindow;
    
    public:
        Window(const WindowParams& windowParams);
        ~Window();

        void Update()   const;
        void EndFrame() const;
        void Close()    const;

        void SetName   (const char* windowName);
        void SetWidth  (const int&  windowWidth ) const;
        void SetHeight (const int&  windowHeight) const;
        void SetSize   (const int&  windowWidth, const int& windowHeight) const;
        void SetPosX   (const int&  windowPosX) const;
        void SetPosY   (const int&  windowPosY) const;
        void SetPos    (const int&  windowPosX, const int& windowPosY) const;
        void SetVsync  (const bool& windowVsync);
        void SetExitKey(const int& windowExitKey) { params.exitKey = windowExitKey; }
        void RemoveExitKey()                      { params.exitKey = -1; }

        const char* GetName()     const { return params.name;   }
        int         GetWidth()    const { return params.width;  }
        int         GetHeight()   const { return params.height; }
        int         GetPosX()     const { return params.posX;   }
        int         GetPosY()     const { return params.posY;   }
        bool        ShouldClose() const;
    };
}
