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

Model::Model(std::string _name)
     : name(std::move(_name))
{
     // TODO: Instance matrices shouldn't be hardcoded
     transforms.resize(10000);
     for (size_t i = 0; i < transforms.size(); i++)
     {
          Transform& t = transforms[i];
          t.SetPosition({ (float)(i / 100), 0, -(float)(i % 100) });
          t.SetRotation({ 0, 1, 0, 0 });
          t.SetScale({ 0.25f });
     }
     Application::Get()->GetGpuData()->CreateData(*this);
}

Model& Model::operator=(Model&& other) noexcept
{
     UniqueID::operator=(std::move(other));
     name       = other.name;                  other.name = "";
     meshes     = std::move(other.meshes);     other.meshes.clear();
     transforms = std::move(other.transforms); other.transforms = {};
     return *this;
}

Model::~Model()
{
     Application::Get()->GetGpuData()->DestroyData(*this);
}

void Model::UpdateMvpBuffer(const uint32_t& currentFrame, const GpuData<Model>* gpuData) const
{
     if (!gpuData) gpuData = Application::Get()->GetGpuData()->GetData(*this);
     Mat4* mappedMatrices = (Mat4*)gpuData->vkMvpBuffersMapped[currentFrame];
     
     // Copy the matrices to buffer memory.
     for (size_t i = 0; i < transforms.size(); i++)
     {
          const Transform& transform = transforms[i];
          const Mat4& model = transform.GetLocalMat();
          memcpy(mappedMatrices + i, &model, sizeof(Mat4));
     }
}


template<> const GpuArray<Model>& GpuDataManager::CreateArray()
{
     if (modelsArray.vkDescriptorSetLayout && modelsArray.vkDescriptorPool) return modelsArray;

     // Get the necessary vulkan resources.
     const VkDevice vkDevice = renderer->GetVkDevice();
     
     // Set the binding of the mvp buffer object.
     VkDescriptorSetLayoutBinding mvpLayoutBinding{};
     mvpLayoutBinding.binding         = 0;
     mvpLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
     mvpLayoutBinding.descriptorCount = 1;
     mvpLayoutBinding.stageFlags      = VK_SHADER_STAGE_ALL;

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
     poolSize.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
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
    const VkDeviceSize bufferSize = resource.transforms.size() * sizeof(Mat4);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     data.vkMvpBuffers[i], data.vkMvpBuffersMemory[i]);

        vkMapMemory(vkDevice, data.vkMvpBuffersMemory[i], 0, bufferSize, 0, &data.vkMvpBuffersMapped[i]);
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
        mvpBufferInfo.range  = bufferSize;
        
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet          = data.vkDescriptorSets[i];
        descriptorWrite.dstBinding      = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo     = &mvpBufferInfo;
        
        vkUpdateDescriptorSets(vkDevice, 1, &descriptorWrite, 0, nullptr);
    }
    
    return data;
}
