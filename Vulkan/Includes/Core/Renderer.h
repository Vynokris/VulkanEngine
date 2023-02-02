#pragma once
#include <cstdint>
#include <optional>
#include <vector>

// Forward declaration of Vulkan types to avoid include.
typedef struct VkInstance_T*            VkInstance;
typedef struct VkPhysicalDevice_T*      VkPhysicalDevice;
typedef struct VkDevice_T*              VkDevice;
typedef struct VkQueue_T*               VkQueue;
typedef struct VkSurfaceKHR_T*          VkSurfaceKHR;
typedef struct VkSwapchainKHR_T*        VkSwapchainKHR;
typedef struct VkImage_T*               VkImage;
typedef struct VkSurfaceCapabilitiesKHR VkSurfaceCapabilitiesKHR;
typedef struct VkSurfaceFormatKHR       VkSurfaceFormatKHR;
typedef struct VkExtent2D               VkExtent2D;
typedef enum   VkPresentModeKHR : int   VkPresentModeKHR;
typedef enum   VkFormat : int           VkFormat;

namespace VulkanUtils
{
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        
        bool IsComplete() const
        {
            return graphicsFamily.has_value()
                && presentFamily .has_value();
        }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR*       capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;

        SwapChainSupportDetails();
        SwapChainSupportDetails(const SwapChainSupportDetails& other);
        ~SwapChainSupportDetails();

        bool IsAdequate() const
        {
            return !formats.empty() && !presentModes.empty();
        }
    };
    
    QueueFamilyIndices      FindQueueFamilies          (const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
    SwapChainSupportDetails QuerySwapChainSupport      (const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
    VkSurfaceFormatKHR      ChooseSwapSurfaceFormat    (const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR        ChooseSwapPresentMode      (const std::vector<VkPresentModeKHR  >& availablePresentModes);
    VkExtent2D              ChooseSwapExtent           (const VkSurfaceCapabilitiesKHR& capabilities);
    bool                    CheckDeviceExtensionSupport(const VkPhysicalDevice& device);
    bool                    IsDeviceSuitable           (const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
}


namespace Core
{
    class Application;

    class Renderer
    {
    private:
        Application*         app;
        VkInstance           vkInstance       = nullptr;
        VkSurfaceKHR         vkSurface        = nullptr;
        VkPhysicalDevice     vkPhysicalDevice = nullptr;
        VkDevice             vkDevice         = nullptr;
        VkQueue              vkGraphicsQueue  = nullptr;
        VkQueue              vkPresentQueue   = nullptr;
        VkSwapchainKHR       vkSwapChain      = nullptr;
        std::vector<VkImage> vkSwapChainImages;
        VkFormat             vkSwapChainImageFormat;
        uint32_t             vkSwapChainWidth, vkSwapChainHeight;
        
    public:
        Renderer(const char* appName, const char* engineName = "No Engine");
        ~Renderer();

    private:
        void CheckValidationLayers() const;
        void CreateVkInstance(const char* appName, const char* engineName);
        void CreateSurface();
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateSwapChain();
    };
}
