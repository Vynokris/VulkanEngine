#pragma once
#include <cstdint>
#include <string>

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
        std::string name;
        int         width     = 0;
        int         height    = 0;
        int         channels  = 0;
        uint32_t    mipLevels = 0;

        VkImage        vkImage       = nullptr;
        VkDeviceMemory vkImageMemory = nullptr;
        VkImageView    vkImageView   = nullptr;
        VkFormat       vkImageFormat;
        
    public:
		bool shouldDelete = false;
        
        Texture() = default;
        Texture(std::string filename);
        Texture(const int& width, const int& height);
        Texture(const Texture& other)      = delete;
        Texture(Texture&&)                 = delete;
        Texture& operator=(const Texture&) = delete;
        Texture& operator=(Texture&&)      = delete;
        ~Texture();

        std::string  GetName()        const { return name; }
        int          GetWidth ()      const { return width; }
        int          GetHeight()      const { return height; }
        uint32_t     GetMipLevels()   const { return mipLevels; }
        VkImageView  GetVkImageView() const { return vkImageView; }
        
    private:
        void GenerateMipmaps() const;
    };
}
