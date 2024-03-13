#pragma once
#include "GraphicsUtils.h"
#include <unordered_map>

#include "UniqueID.h"

namespace Resources { class Texture; class Material; class Mesh; class Model; class Light; }

namespace Core
{
    class Renderer;

    template<typename T> struct GpuData { GpuData() = delete; };

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

    template<typename T> struct DataPool
    {
        VkDescriptorSetLayout vkDescriptorSetLayout = nullptr;
        VkDescriptorPool      vkDescriptorPool      = nullptr;
    };

    template<> struct DataPool<Resources::Light>
    {
        VkDescriptorSetLayout vkDescriptorSetLayout = nullptr;
        VkDescriptorPool      vkDescriptorPool      = nullptr;
        VkDescriptorSet       vkDescriptorSet       = nullptr;
        VkBuffer              vkBuffer              = nullptr;
        VkDeviceMemory        vkBufferMemory        = nullptr;
    };

    class GpuDataManager
    {
    private:
        Renderer* renderer;
        
        DataPool<Resources::Material> materialPool;
        DataPool<Resources::Model>    modelPool;
        DataPool<Resources::Light>    lightPool;
        
        std::unordered_map<uid_t, GpuData<Resources::Texture>>  textures;
        std::unordered_map<uid_t, GpuData<Resources::Material>> materials;
        std::unordered_map<uid_t, GpuData<Resources::Mesh>>     meshes;
        std::unordered_map<uid_t, GpuData<Resources::Model>>    models;

    public:
        GpuDataManager(Renderer* _renderer) : renderer(_renderer) {}
        GpuDataManager(const GpuDataManager&)            = delete;
        GpuDataManager(GpuDataManager&&)                 = delete;
        GpuDataManager& operator=(const GpuDataManager&) = delete;
        GpuDataManager& operator=(GpuDataManager&&)      = delete;
        ~GpuDataManager();

        template<typename T> const DataPool<T>& CreatePool();
        template<typename T> const GpuData<T>&  CreateData(const T& resource);
        
        template<typename T> void DestroyPool();
        template<typename T> void DestroyData(const T& resource);

        template<typename T> const DataPool<T>& GetPool() const;
        template<typename T> const GpuData<T>&  GetData(const T& resource) const;
    };
}
