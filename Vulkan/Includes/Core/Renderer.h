#pragma once
#include <cstdint>
#include <optional>

// Forward declaration of Vulkan types to avoid include.
typedef struct VkInstance_T*       VkInstance;
typedef struct VkPhysicalDevice_T* VkPhysicalDevice;
typedef struct VkDevice_T*         VkDevice;
typedef struct VkQueue_T*          VkQueue;
typedef struct VkSurfaceKHR_T*     VkSurfaceKHR;


namespace Core
{
    class Application;

    struct VkQueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        
        bool IsComplete() const
        {
            return graphicsFamily.has_value()
                && presentFamily .has_value();
        }
    };
    
    class Renderer
    {
    private:
        Application*     app;
        VkInstance       vkInstance;
        VkSurfaceKHR     vkSurface;
        VkPhysicalDevice vkPhysicalDevice;
        VkDevice         vkDevice;
        VkQueue          vkGraphicsQueue;
        VkQueue          vkPresentQueue;
        
    public:
        Renderer(const char* appName, const char* engineName = "No Engine");
        ~Renderer();

    private:
        void CheckValidationLayers() const;
        void CreateVkInstance(const char* appName, const char* engineName);
        void CreateSurface();
        void PickPhysicalDevice();
        void CreateLogicalDevice();

        VkQueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device) const;
        bool IsDeviceSuitable(const VkPhysicalDevice& device);
    };
}
