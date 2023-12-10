#pragma once
#include <unordered_map>
#include <string>

typedef struct VkDescriptorPool_T*      VkDescriptorPool;
typedef struct VkDescriptorSetLayout_T* VkDescriptorSetLayout;
typedef struct VkDescriptorSet_T*       VkDescriptorSet;
typedef struct VkBuffer_T*              VkBuffer;
typedef struct VkDeviceMemory_T*        VkDeviceMemory;

namespace Maths
{
    class Transform;
}

namespace Resources
{
    class Camera;
    class Model;
    class Mesh;
    class Texture;
}

namespace Core
{
    class Application;
    class Engine;
    class Logger;
    
    class UserInterface
    {
    private:
        Application*     app              = nullptr;
        Engine*          engine           = nullptr;
        VkDescriptorPool vkDescriptorPool = nullptr;

        // Pointers to engine resources.
        Resources::Camera* camera = nullptr;
        std::unordered_map<std::string, Resources::Model>*   models   = nullptr;
        std::unordered_map<std::string, Resources::Texture>* textures = nullptr;
        
    public:
        UserInterface();
        UserInterface(const UserInterface&)            = delete;
        UserInterface(UserInterface&&)                 = delete;
        UserInterface& operator=(const UserInterface&) = delete;
        UserInterface& operator=(UserInterface&&)      = delete;
        ~UserInterface();

        void SetResourceRefs(Resources::Camera* _camera, std::unordered_map<std::string, Resources::Model>* _models, std::unordered_map<std::string, Resources::Texture>* _textures);
        void Render() const;

    private:
        void CreateDescriptorPool();
        void InitImGui       () const;
        void UploadImGuiFonts() const;

        void ShowStatsWindow    () const;
        void ShowLogsWindow     () const;
        void ShowResourcesWindow() const;
        
        void NewFrame   () const;
        void RenderFrame() const;
    };
}