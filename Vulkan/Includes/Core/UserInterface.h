#pragma once
#include <vector>

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
    
    class UserInterface
    {
    private:
        Application*     app;
        VkDescriptorPool vkDescriptorPool = nullptr;

        // Pointers to engine resources.
        Resources::Camera*                camera   = nullptr;
        std::vector<Resources::Model*>*   models   = nullptr;
        std::vector<Resources::Mesh*>*    meshes   = nullptr;
        std::vector<Resources::Texture*>* textures = nullptr;
        
    public:
        UserInterface();
        ~UserInterface();

        void SetResourceRefs(Resources::Camera* _camera, std::vector<Resources::Model*>* _models, std::vector<Resources::Mesh*>* _meshes, std::vector<Resources::Texture*>* _textures);
        void Render() const;

    private:
        void CreateDescriptorPool();
        void InitImGui() const;
        void UploadImGuiFonts() const;
        
        void ShowTransformUi(Maths::Transform& transform) const;

        void ShowStatsWindow()  const;
        void ShowSceneWindow()  const;
        void ShowLoaderWindow() const;
        
        void NewFrame()    const;
        void RenderFrame() const;
    };
}