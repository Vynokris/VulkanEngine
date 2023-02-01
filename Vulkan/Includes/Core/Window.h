#pragma once
typedef struct GLFWwindow GLFWwindow;

namespace Core
{
    class Window
    {
    private:
        const char* name;
        int width, height, posX, posY;
        bool vsync;
        int exitKey;
        GLFWwindow* glfwWindow;
    
    public:
        Window(const char* windowName, const int& windowWidth = -1, const int& windowHeight = -1, const int& windowPosX = 50, const int& windowPosY = 70, const bool& windowVsync = false);
        ~Window();

        void Update()   const;
        void EndFrame() const;

        void SetName   (const char* windowName);
        void SetWidth  (const int&  windowWidth ) const;
        void SetHeight (const int&  windowHeight) const;
        void SetSize   (const int&  windowWidth, const int& windowHeight) const;
        void SetPosX   (const int&  windowPosX) const;
        void SetPosY   (const int&  windowPosY) const;
        void SetPos    (const int&  windowPosX, const int& windowPosY) const;
        void SetVsync  (const bool& windowVsync);
        void SetExitKey(const int& windowExitKey) { exitKey = windowExitKey; }
        void RemoveExitKey()                      { exitKey = -1; }

        const char* GetName()     const { return name;   }
        int         GetWidth()    const { return width;  }
        int         GetHeight()   const { return height; }
        int         GetPosX()     const { return posX;   }
        int         GetPosY()     const { return posY;   }
        bool        ShouldClose() const;
    };
}
