#include "Core/GpuDataManager.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "Core/Engine.h"
#include "Resources/Texture.h"
#include "Resources/Material.h"
#include "Resources/Mesh.h"
#include "Resources/Model.h"
#include "Resources/Light.h"
#include <vulkan/vulkan_core.h>
using namespace Core;
using namespace Resources;
using namespace GraphicsUtils;

GpuDataManager::~GpuDataManager()
{
    
}

template<> void GpuDataManager::DestroyArray<Material>()
{
    const VkDevice vkDevice = renderer->GetVkDevice();
    if (materialArray.vkDescriptorSetLayout) vkDestroyDescriptorSetLayout(vkDevice, materialArray.vkDescriptorSetLayout, nullptr);
    if (materialArray.vkDescriptorPool)      vkDestroyDescriptorPool     (vkDevice, materialArray.vkDescriptorPool,      nullptr);
}

template<> void GpuDataManager::DestroyArray<Model>()
{
    const VkDevice vkDevice = renderer->GetVkDevice();
    if (modelArray.vkDescriptorSetLayout) vkDestroyDescriptorSetLayout(vkDevice, modelArray.vkDescriptorSetLayout, nullptr);
    if (modelArray.vkDescriptorPool)      vkDestroyDescriptorPool     (vkDevice, modelArray.vkDescriptorPool,      nullptr);
}

template<> void GpuDataManager::DestroyArray<Light>()
{
    const VkDevice vkDevice = renderer->GetVkDevice();
    if (lightArray.vkDescriptorSetLayout) vkDestroyDescriptorSetLayout(vkDevice, lightArray.vkDescriptorSetLayout, nullptr);
    if (lightArray.vkDescriptorPool)      vkDestroyDescriptorPool     (vkDevice, lightArray.vkDescriptorPool,      nullptr);
    if (lightArray.vkBuffer)              vkDestroyBuffer(vkDevice, lightArray.vkBuffer,       nullptr);
    if (lightArray.vkBufferMemory)        vkFreeMemory   (vkDevice, lightArray.vkBufferMemory, nullptr);
}

template<> void GpuDataManager::DestroyData(const Texture& resource)
{
    if (!CheckData(resource)) return;
    const VkDevice vkDevice = renderer->GetVkDevice();
    const GpuData<Texture>& data = textures.at(resource.GetID());
    if (data.vkImage      ) vkDestroyImage    (vkDevice, data.vkImage,       nullptr);
    if (data.vkImageMemory) vkFreeMemory      (vkDevice, data.vkImageMemory, nullptr);
    if (data.vkImageView  ) vkDestroyImageView(vkDevice, data.vkImageView,   nullptr);
    textures.erase(resource.GetID());
}

template<> void GpuDataManager::DestroyData(const Material& resource)
{
    if (!CheckData(resource)) return;
    const VkDevice vkDevice = renderer->GetVkDevice();
    const GpuData<Material>& data = materials.at(resource.GetID());
    if (data.vkDataBuffer)       vkDestroyBuffer(vkDevice, data.vkDataBuffer,       nullptr);
    if (data.vkDataBufferMemory) vkFreeMemory   (vkDevice, data.vkDataBufferMemory, nullptr);
    materials.erase(resource.GetID());
}

template<> void GpuDataManager::DestroyData(const Mesh& resource)
{
    if (!CheckData(resource)) return;
    const VkDevice vkDevice = renderer->GetVkDevice();
    const GpuData<Mesh>& data = meshes.at(resource.GetID());
    if (data.vkIndexBuffer       ) vkDestroyBuffer(vkDevice, data.vkIndexBuffer,        nullptr);
    if (data.vkIndexBufferMemory ) vkFreeMemory   (vkDevice, data.vkIndexBufferMemory,  nullptr);
    if (data.vkVertexBuffer      ) vkDestroyBuffer(vkDevice, data.vkVertexBuffer,       nullptr);
    if (data.vkVertexBufferMemory) vkFreeMemory   (vkDevice, data.vkVertexBufferMemory, nullptr);
    materials.erase(resource.GetID());
}

template<> void GpuDataManager::DestroyData(const Model& resource)
{
    if (!CheckData(resource)) return;
    const VkDevice vkDevice = renderer->GetVkDevice();
    const GpuData<Model>& data = models.at(resource.GetID());
    for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (data.vkMvpBuffers[i])       vkDestroyBuffer(vkDevice, data.vkMvpBuffers[i],       nullptr);
        if (data.vkMvpBuffersMemory[i]) vkFreeMemory   (vkDevice, data.vkMvpBuffersMemory[i], nullptr);
    }
    materials.erase(resource.GetID());
}

template<> bool GpuDataManager::CheckArray<Material>() const { return materialArray.vkDescriptorPool && materialArray.vkDescriptorSetLayout; }
template<> bool GpuDataManager::CheckArray<Model>()    const { return modelArray   .vkDescriptorPool && modelArray   .vkDescriptorSetLayout; }
template<> bool GpuDataManager::CheckArray<Light>()    const { return lightArray   .vkDescriptorPool && lightArray   .vkDescriptorSetLayout
                                                                   && lightArray.vkDescriptorSet && lightArray.vkBuffer && lightArray.vkBufferMemory; }

template<> bool GpuDataManager::CheckData(const Texture&  resource) const { const uid_t id = resource.GetID(); return id != UniqueID::unassigned && textures .find(id) != textures .end(); }
template<> bool GpuDataManager::CheckData(const Material& resource) const { const uid_t id = resource.GetID(); return id != UniqueID::unassigned && materials.find(id) != materials.end(); }
template<> bool GpuDataManager::CheckData(const Mesh&     resource) const { const uid_t id = resource.GetID(); return id != UniqueID::unassigned && meshes   .find(id) != meshes   .end(); }
template<> bool GpuDataManager::CheckData(const Model&    resource) const { const uid_t id = resource.GetID(); return id != UniqueID::unassigned && models   .find(id) != models   .end(); }

template<> const GpuArray<Material>& GpuDataManager::GetArray() const { return materialArray; }
template<> const GpuArray<Model>&    GpuDataManager::GetArray() const { return modelArray;    }
template<> const GpuArray<Light>&    GpuDataManager::GetArray() const { return lightArray;    }

template<> const GpuData<Texture>& GpuDataManager::GetData(const Texture& resource) const
{
    const uid_t id = resource.GetID();
    if (id == UniqueID::unassigned || textures.find(id) == textures.end()) {
        LogError(LogType::Resources, "No texture with ID " + std::to_string(id));
        throw std::runtime_error("RESOURCE_ID_NOT_FOUND");
    }
    return textures.at(id);
}

template<> const GpuData<Material>& GpuDataManager::GetData(const Material& resource) const
{
    const uid_t id = resource.GetID();
    if (id == UniqueID::unassigned || materials.find(id) == materials.end()) {
        LogError(LogType::Resources, "No material with ID " + std::to_string(id));
        throw std::runtime_error("RESOURCE_ID_NOT_FOUND");
    }
    return materials.at(id);
}

template<> const GpuData<Mesh>& GpuDataManager::GetData(const Mesh& resource) const
{
    const uid_t id = resource.GetID();
    if (id == UniqueID::unassigned || meshes.find(id) == meshes.end()) {
        LogError(LogType::Resources, "No mesh with ID " + std::to_string(id));
        throw std::runtime_error("RESOURCE_ID_NOT_FOUND");
    }
    return meshes.at(id);
}

template<> const GpuData<Model>& GpuDataManager::GetData(const Model& resource) const
{
    const uid_t id = resource.GetID();
    if (id == UniqueID::unassigned || models.find(id) == models.end()) {
        LogError(LogType::Resources, "No model with ID " + std::to_string(id));
        throw std::runtime_error("RESOURCE_ID_NOT_FOUND");
    }
    return models.at(id);
}
