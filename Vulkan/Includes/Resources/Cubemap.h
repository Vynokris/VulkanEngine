#pragma once
#include "Core/UniqueID.h"
#include <array>
#include <string>

namespace Resources
{
    class Cubemap : public UniqueID
    {
    private:
        std::array<std::string,    6> names;
        std::array<unsigned char*, 6> pixels;
        int      width     = 0;
        int      height    = 0;
        int      channels  = 0;
        uint32_t mipLevels = 0;
        
    public:
        Cubemap() = default;
        Cubemap(std::array<std::string, 6> filenames);
        Cubemap(const Cubemap&) = delete;
        Cubemap(Cubemap&&) noexcept;
        Cubemap& operator=(const Cubemap&) = delete;
        Cubemap& operator=(Cubemap&&) noexcept;
        ~Cubemap();
        
        std::string    GetName(uint32_t face)   const { return names[face]; }
        unsigned char* GetPixels(uint32_t face) const { return pixels[face]; }
        int            GetWidth ()              const { return width; }
        int            GetHeight()              const { return height; }
        uint32_t       GetMipLevels()           const { return mipLevels; }
    };
}
