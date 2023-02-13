#pragma once

typedef struct VkImage_T*        VkImage;
typedef struct VkDeviceMemory_T* VkDeviceMemory;
typedef struct VkImageView_T*    VkImageView;

namespace Core
{
    class Renderer;
}

namespace Resources
{
    class Texture
    {
    private:
        const char* filename = "";
        int width  = 0;
        int height = 0;
        int channels = 0;

        VkImage        vkImage       = nullptr;
        VkDeviceMemory vkImageMemory = nullptr;
        VkImageView    vkImageView   = nullptr;
        
    public:
        Texture() = default;
        Texture(const char* name, const Core::Renderer* renderer = nullptr);
        Texture(const int& width, const int& height, const Core::Renderer* renderer = nullptr);

        void Load(const char* name, const Core::Renderer* renderer = nullptr);
        void Load(const int& width, const int& height, const Core::Renderer* renderer = nullptr);
        
        ~Texture();

        VkImageView GetVkImageView() const { return vkImageView; }
    };
}
