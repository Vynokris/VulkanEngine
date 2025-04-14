#include "Core/Application.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "Core/GpuDataManager.h"
#include "Core/Engine.h"
#include "Resources/Model.h"
#include "Resources/Camera.h"
#include "Resources/Mesh.h"
#include "Maths/Vertex.h"
#include <vulkan/vulkan.h>

using namespace Core;
using namespace GraphicsUtils;
using namespace Resources;
using namespace Maths;

Model::Model(std::string _name, Transform _transform)
     : name(std::move(_name)), transform(std::move(_transform))
{
     transform.SetRotation({ 0, 1, 0, 0 });
     Application::Get()->GetGpuData()->CreateData(*this);
}

Model& Model::operator=(Model&& other) noexcept
{
     UniqueID::operator=(std::move(other));
     name      = other.name;                 other.name = "";
     meshes    = std::move(other.meshes);    other.meshes.clear();
     transform = std::move(other.transform); other.transform = {};
     return *this;
}

Model::~Model()
{
     Application::Get()->GetGpuData()->DestroyData(*this);
}

void Model::UpdateMvpBuffer(const Camera& camera, const uint32_t& currentFrame, const GpuData<Model>* gpuData) const
{
     if (!gpuData) gpuData = &Application::Get()->GetGpuData()->GetData(*this);
     
     // Copy the matrices to buffer memory.
     const MvpBuffer mvp = { transform.GetLocalMat(), transform.GetLocalMat() * camera.GetViewMat() * camera.GetProjMat() };
     memcpy(Application::Get()->GetGpuData()->GetData(*this).vkMvpBuffersMapped[currentFrame], &mvp, sizeof(mvp));
}


template<> const GpuArray<Model>& GpuDataManager::CreateArray()
{
     if (modelsArray.vkDescriptorSetLayout && modelsArray.vkDescriptorPool) return modelsArray;

     // Get the necessary vulkan resources.
     const VkDevice vkDevice = renderer->GetVkDevice();
     
     // Set the binding of the mvp buffer object.
     VkDescriptorSetLayoutBinding mvpLayoutBinding{};
     mvpLayoutBinding.binding         = 0;
     mvpLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
     mvpLayoutBinding.descriptorCount = 1;
     mvpLayoutBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

     // Create the descriptor set layout.
     VkDescriptorSetLayoutCreateInfo layoutInfo{};
     layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
     layoutInfo.bindingCount = 1;
     layoutInfo.pBindings    = &mvpLayoutBinding;
     if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &modelsArray.vkDescriptorSetLayout) != VK_SUCCESS) {
          LogError(LogType::Vulkan, "Failed to create descriptor set layout.");
          throw std::runtime_error("VULKAN_DESCRIPTOR_SET_LAYOUT_ERROR");
     }

     // Set the type and number of descriptors.
     VkDescriptorPoolSize poolSize{};
     poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
     poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT * Engine::MAX_MODELS;

     // Create the descriptor pool.
     VkDescriptorPoolCreateInfo poolInfo{};
     poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
     poolInfo.poolSizeCount = 1;
     poolInfo.pPoolSizes    = &poolSize;
     poolInfo.maxSets       = MAX_FRAMES_IN_FLIGHT * Engine::MAX_MODELS;
     if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &modelsArray.vkDescriptorPool) != VK_SUCCESS) {
          LogError(LogType::Vulkan, "Failed to create descriptor pool.");
          throw std::runtime_error("VULKAN_DESCRIPTOR_POOL_ERROR");
     }

     return modelsArray;
}

template<> const GpuData<Model>& GpuDataManager::CreateData(const Model& resource)
{
     if (resource.GetID() == UniqueID::unassigned) {
          LogError(LogType::Resources, "Can't create GPU data from unassigned resource.");
          throw std::runtime_error("RESOURCE_UNASSIGNED_ERROR");
     }
    GpuData<Model>& data = models.emplace(std::make_pair(resource.GetID(), GpuData<Model>())).first->second;

    // Get necessary vulkan resources.
    const VkDevice         vkDevice         = renderer->GetVkDevice();
    const VkPhysicalDevice vkPhysicalDevice = renderer->GetVkPhysicalDevice();

    // Create the buffers.
    constexpr VkDeviceSize mvpSize = sizeof(Maths::MvpBuffer);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        CreateBuffer(vkDevice, vkPhysicalDevice, mvpSize,
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     data.vkMvpBuffers[i], data.vkMvpBuffersMemory[i]);

        vkMapMemory(vkDevice, data.vkMvpBuffersMemory[i], 0, mvpSize, 0, &data.vkMvpBuffersMapped[i]);
    }

    // Allocate the descriptor sets.
    const std::vector layouts(MAX_FRAMES_IN_FLIGHT, modelsArray.vkDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = modelsArray.vkDescriptorPool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts        = layouts.data();
    if (vkAllocateDescriptorSets(vkDevice, &allocInfo, data.vkDescriptorSets) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to allocate descriptor sets.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_SET_ALLOCATION_ERROR");
    }

    // Populate the descriptor sets.
    for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo mvpBufferInfo{};
        mvpBufferInfo.buffer = data.vkMvpBuffers[i];
        mvpBufferInfo.offset = 0;
        mvpBufferInfo.range  = mvpSize;
        
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet          = data.vkDescriptorSets[i];
        descriptorWrite.dstBinding      = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo     = &mvpBufferInfo;
        
        vkUpdateDescriptorSets(vkDevice, 1, &descriptorWrite, 0, nullptr);
    }
    
    return data;
}
