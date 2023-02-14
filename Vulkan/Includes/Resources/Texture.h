#pragma once
#include <cstdint>

typedef struct VkImage_T*        VkImage;
typedef struct VkDeviceMemory_T* VkDeviceMemory;
typedef struct VkImageView_T*    VkImageView;
typedef enum   VkFormat : int    VkFormat;

namespace Core
{
    class Renderer;
}

namespace Resources
{
    class Texture
    {
    private:
        const char* filename  = "";
        int         width     = 0;
        int         height    = 0;
        int         channels  = 0;
        uint32_t    mipLevels = 0;

        VkImage        vkImage       = nullptr;
        VkDeviceMemory vkImageMemory = nullptr;
        VkImageView    vkImageView   = nullptr;
        VkFormat       vkImageFormat;
        
    public:
        Texture() = default;
        Texture(const char* name);
        Texture(const int& width, const int& height);
        
        ~Texture();

        int GetWidth () const { return width;  }
        int GetHeight() const { return height; }
        uint32_t    GetMipLevels()   const { return mipLevels; }
        VkImageView GetVkImageView() const { return vkImageView; }
        
    private:
        void GenerateMipmaps() const;
    };
}
