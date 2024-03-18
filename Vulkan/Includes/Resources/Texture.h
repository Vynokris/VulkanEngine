#pragma once
#include "Core/UniqueID.h"
#include <cstdint>
#include <string>

namespace Core { class Renderer; }
namespace Resources
{
    class Texture : public UniqueID
    {
    private:
        std::string name;
        int         width     = 0;
        int         height    = 0;
        int         channels  = 0;
        uint32_t    mipLevels = 0;
        unsigned char* pixels = nullptr;
        
    public:
        Texture() = default;
        Texture(std::string filename);
        Texture(const Texture&) = delete;
        Texture(Texture&&) noexcept;
        Texture& operator=(const Texture&) = delete;
        Texture& operator=(Texture&&) noexcept;
        ~Texture();

        std::string    GetName()        const { return name; }
        int            GetWidth ()      const { return width; }
        int            GetHeight()      const { return height; }
        uint32_t       GetMipLevels()   const { return mipLevels; }
        unsigned char* GetPixels()      const { return pixels; }
    };
}
