#pragma once
#include "UniqueID.h"
#include "GraphicsUtils.h"
#include <unordered_map>

namespace Resources { class Texture; class Material; class Mesh; class Model; class Light; }

namespace Core
{
    class Renderer;

    template<typename> struct GpuData { GpuData() = delete; };

    template<> struct GpuData<Resources::Texture>
    {
        VkImage        vkImage       = nullptr;
        VkDeviceMemory vkImageMemory = nullptr;
        VkImageView    vkImageView   = nullptr;
        VkFormat       vkImageFormat;
    };

    template<> struct GpuData<Resources::Material>
    {
        VkDescriptorSet vkDescriptorSet    = nullptr;
        VkBuffer        vkDataBuffer       = nullptr;
        VkDeviceMemory  vkDataBufferMemory = nullptr;
    };

    template<> struct GpuData<Resources::Mesh>
    {
        VkBuffer       vkVertexBuffer       = nullptr;
        VkDeviceMemory vkVertexBufferMemory = nullptr;
        VkBuffer       vkIndexBuffer        = nullptr;
        VkDeviceMemory vkIndexBufferMemory  = nullptr;
    };

    template<> struct GpuData<Resources::Model>
    {
        VkDescriptorSet vkDescriptorSets  [GraphicsUtils::MAX_FRAMES_IN_FLIGHT] = { nullptr };
        VkBuffer        vkMvpBuffers      [GraphicsUtils::MAX_FRAMES_IN_FLIGHT] = { nullptr };
        VkDeviceMemory  vkMvpBuffersMemory[GraphicsUtils::MAX_FRAMES_IN_FLIGHT] = { nullptr };
        void*           vkMvpBuffersMapped[GraphicsUtils::MAX_FRAMES_IN_FLIGHT] = { nullptr };
    };

    template<typename> struct GpuArray
    {
        VkDescriptorSetLayout vkDescriptorSetLayout = nullptr;
        VkDescriptorPool      vkDescriptorPool      = nullptr;
    };

    template<> struct GpuArray<Resources::Light>
    {
        VkDescriptorSetLayout vkDescriptorSetLayout = nullptr;
        VkDescriptorPool      vkDescriptorPool      = nullptr;
        VkDescriptorSet       vkDescriptorSet       = nullptr;
        VkBuffer              vkBuffer              = nullptr;
        VkDeviceMemory        vkBufferMemory        = nullptr;
        void*                 vkBufferMapped        = nullptr;
        VkDeviceSize          vkBufferSize          = 0;
    };

    class GpuDataManager
    {
    private:
        Renderer* renderer;
        
        GpuArray<Resources::Material> materialsArray;
        GpuArray<Resources::Model>    modelsArray;
        GpuArray<Resources::Light>    lightsArray;
        
        std::unordered_map<uid_t, GpuData<Resources::Texture>>  textures;
        std::unordered_map<uid_t, GpuData<Resources::Material>> materials;
        std::unordered_map<uid_t, GpuData<Resources::Mesh>>     meshes;
        std::unordered_map<uid_t, GpuData<Resources::Model>>    models;

    public:
        GpuDataManager() = default;
        GpuDataManager(const GpuDataManager&)            = delete;
        GpuDataManager(GpuDataManager&&)                 = delete;
        GpuDataManager& operator=(const GpuDataManager&) = delete;
        GpuDataManager& operator=(GpuDataManager&&)      = delete;
        ~GpuDataManager();

        void SetRendererPtr(Renderer* _renderer) { renderer = _renderer; }
        
        template<typename T> const GpuArray<T>& CreateArray();
        template<typename T> const GpuData <T>& CreateData(const T& resource);
        
        template<typename T> void DestroyArray();
        template<typename T> void DestroyData(const T& resource);

        template<typename T> bool CheckArray() const;
        template<typename T> bool CheckData(const T& resource) const;

        template<typename T> const GpuArray<T>& GetArray() const;
        template<typename T> const GpuData<T>*  GetData(const T& resource) const;
    };
    
    template<> const GpuArray<Resources::Material>& GpuDataManager::CreateArray<Resources::Material>();
    template<> const GpuArray<Resources::Model   >& GpuDataManager::CreateArray<Resources::Model   >();
    template<> const GpuArray<Resources::Light   >& GpuDataManager::CreateArray<Resources::Light   >();
    template<> const GpuData <Resources::Texture >& GpuDataManager::CreateData(const Resources::Texture&  resource);
    template<> const GpuData <Resources::Material>& GpuDataManager::CreateData(const Resources::Material& resource);
    template<> const GpuData <Resources::Model   >& GpuDataManager::CreateData(const Resources::Model&    resource);
    template<> const GpuData <Resources::Mesh    >& GpuDataManager::CreateData(const Resources::Mesh&     resource);
    
    template<> void GpuDataManager::DestroyArray<Resources::Material>();
    template<> void GpuDataManager::DestroyArray<Resources::Model   >();
    template<> void GpuDataManager::DestroyArray<Resources::Light   >();
    template<> void GpuDataManager::DestroyData(const Resources::Texture&  resource);
    template<> void GpuDataManager::DestroyData(const Resources::Material& resource);
    template<> void GpuDataManager::DestroyData(const Resources::Model&    resource);
    template<> void GpuDataManager::DestroyData(const Resources::Mesh&     resource);
    
    template<> bool GpuDataManager::CheckArray<Resources::Material>() const;
    template<> bool GpuDataManager::CheckArray<Resources::Model   >() const;
    template<> bool GpuDataManager::CheckArray<Resources::Light   >() const;
    template<> bool GpuDataManager::CheckData(const Resources::Texture&  resource) const;
    template<> bool GpuDataManager::CheckData(const Resources::Material& resource) const;
    template<> bool GpuDataManager::CheckData(const Resources::Model&    resource) const;
    template<> bool GpuDataManager::CheckData(const Resources::Mesh&     resource) const;
    
    template<> const GpuArray<Resources::Material>& GpuDataManager::GetArray<Resources::Material>() const;
    template<> const GpuArray<Resources::Model   >& GpuDataManager::GetArray<Resources::Model   >() const;
    template<> const GpuArray<Resources::Light   >& GpuDataManager::GetArray<Resources::Light   >() const;
    template<> const GpuData <Resources::Texture >* GpuDataManager::GetData(const Resources::Texture&  resource) const;
    template<> const GpuData <Resources::Material>* GpuDataManager::GetData(const Resources::Material& resource) const;
    template<> const GpuData <Resources::Model   >* GpuDataManager::GetData(const Resources::Model&    resource) const;
    template<> const GpuData <Resources::Mesh    >* GpuDataManager::GetData(const Resources::Mesh&     resource) const;
}