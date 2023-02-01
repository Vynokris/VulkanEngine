#pragma once
#include <cstdint>
#include <optional>

// Forward declaration of vulkan types to avoid include.
typedef struct VkInstance_T*       VkInstance;
typedef struct VkPhysicalDevice_T* VkPhysicalDevice;
typedef struct VkDevice_T*         VkDevice;
typedef struct VkQueue_T*          VkQueue;


namespace Core
{
    class Application;

    struct VkQueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        
        bool IsComplete() const
        {
            return graphicsFamily.has_value();
        }
    };
    
    class Renderer
    {
    private:
        Application*        app;
        VkInstance_T*       vkInstance;
        VkPhysicalDevice_T* vkPhysicalDevice;
        VkDevice_T*         vkDevice;
        VkQueue_T*          vkGraphicsQueue;
        
    public:
        Renderer(const char* appName, const char* engineName = "No Engine");
        ~Renderer();

    private:
        void CheckValidationLayers() const;
        void CreateVkInstance(const char* appName, const char* engineName);
        void PickPhysicalDevice();
        void CreateLogicalDevice();

        VkQueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device) const;
        bool IsDeviceSuitable(const VkPhysicalDevice& device);
    };
}
