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

void Model::UpdateTransformBuffer(const uint32_t& currentFrame, const GpuData<Model>* gpuData) const
{
     if (!gpuData) gpuData = Application::Get()->GetGpuData()->GetData(*this);
     Mat4* mappedMatrices = (Mat4*)gpuData->vkTransformBuffersMapped[currentFrame];
     
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
     if (modelsArray.vkComputeDescriptorSetLayout  && modelsArray.vkComputeDescriptorPool &&
         modelsArray.vkGraphicsDescriptorSetLayout && modelsArray.vkGraphicsDescriptorPool)
          return modelsArray;

     // Get the necessary vulkan resources.
     const VkDevice vkDevice = renderer->GetVkDevice();

     // Compute layout.
     {
          // Set the binding of the selected sections and indirection buffers.
          VkDescriptorSetLayoutBinding layoutBindings[2];
          layoutBindings[0].binding         = 0;
          layoutBindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
          layoutBindings[0].descriptorCount = 1;
          layoutBindings[0].stageFlags      = VK_SHADER_STAGE_ALL;
          layoutBindings[1].binding         = 1;
          layoutBindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
          layoutBindings[1].descriptorCount = 1;
          layoutBindings[1].stageFlags      = VK_SHADER_STAGE_ALL;

          // Create the descriptor set layout.
          VkDescriptorSetLayoutCreateInfo layoutInfo{};
          layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
          layoutInfo.bindingCount = 2;
          layoutInfo.pBindings    = layoutBindings;
          if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &modelsArray.vkComputeDescriptorSetLayout) != VK_SUCCESS) {
               LogError(LogType::Vulkan, "Failed to create descriptor set layout.");
               throw std::runtime_error("VULKAN_DESCRIPTOR_SET_LAYOUT_ERROR");
          }

          // Set the type and number of descriptors.
          VkDescriptorPoolSize poolSize{};
          poolSize.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
          poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT * Engine::MAX_MODELS * 2;

          // Create the descriptor pool.
          VkDescriptorPoolCreateInfo poolInfo{};
          poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
          poolInfo.poolSizeCount = 1;
          poolInfo.pPoolSizes    = &poolSize;
          poolInfo.maxSets       = MAX_FRAMES_IN_FLIGHT * Engine::MAX_MODELS * 2;
          if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &modelsArray.vkComputeDescriptorPool) != VK_SUCCESS) {
               LogError(LogType::Vulkan, "Failed to create descriptor pool.");
               throw std::runtime_error("VULKAN_DESCRIPTOR_POOL_ERROR");
          }
     }
     
     // Graphics layout.
     {
          // Set the binding of the transform buffer object.
          VkDescriptorSetLayoutBinding layoutBinding{};
          layoutBinding.binding         = 0;
          layoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
          layoutBinding.descriptorCount = 1;
          layoutBinding.stageFlags      = VK_SHADER_STAGE_ALL;

          // Create the descriptor set layout.
          VkDescriptorSetLayoutCreateInfo layoutInfo{};
          layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
          layoutInfo.bindingCount = 1;
          layoutInfo.pBindings    = &layoutBinding;
          if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &modelsArray.vkGraphicsDescriptorSetLayout) != VK_SUCCESS) {
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
          if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &modelsArray.vkGraphicsDescriptorPool) != VK_SUCCESS) {
               LogError(LogType::Vulkan, "Failed to create descriptor pool.");
               throw std::runtime_error("VULKAN_DESCRIPTOR_POOL_ERROR");
          }
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

     // Get resource params.
    const VkDeviceSize bufferElemCount = resource.transforms.size();
    bool hasLODs = false;
    for (const Mesh& mesh : resource.GetMeshes())
    {
        if (mesh.HasSections())
        {
            hasLODs = true;
            break;
        }
    }

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        // Create the model transform buffers.
        CreateBuffer(vkDevice, vkPhysicalDevice, bufferElemCount * sizeof(Mat4),
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     data.vkTransformBuffers[i], data.vkTransformBuffersMemory[i]);

        vkMapMemory(vkDevice, data.vkTransformBuffersMemory[i], 0, bufferElemCount * sizeof(Mat4), 0, &data.vkTransformBuffersMapped[i]);

         // Create buffers to hold selected LODs and indirection IDs (written in compute).
         if (hasLODs)
         {
              CreateBuffer(vkDevice, vkPhysicalDevice, bufferElemCount * sizeof(uint32_t),
                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                           data.vkSelectedSectionsBuffers[i], data.vkSelectedSectionsBuffersMemory[i]);
              CreateBuffer(vkDevice, vkPhysicalDevice, bufferElemCount * sizeof(uint32_t),
                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                           data.vkIndirectionBuffers[i], data.vkIndirectionBuffersMemory[i]);
         }
    }

     // Compute descriptor sets.
     if (hasLODs)
     {
          // Allocate the descriptor sets.
          const std::vector layouts(MAX_FRAMES_IN_FLIGHT, modelsArray.vkComputeDescriptorSetLayout);
          VkDescriptorSetAllocateInfo allocInfo{};
          allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
          allocInfo.descriptorPool     = modelsArray.vkComputeDescriptorPool;
          allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
          allocInfo.pSetLayouts        = layouts.data();
          if (vkAllocateDescriptorSets(vkDevice, &allocInfo, data.vkIndirectDescriptorSets) != VK_SUCCESS) {
               LogError(LogType::Vulkan, "Failed to allocate descriptor sets.");
               throw std::runtime_error("VULKAN_DESCRIPTOR_SET_ALLOCATION_ERROR");
          }

          // Populate the descriptor sets.
          for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
          {
               VkDescriptorBufferInfo bufferInfos[2] = {};
               bufferInfos[0].buffer = data.vkSelectedSectionsBuffers[i];
               bufferInfos[0].offset = 0;
               bufferInfos[0].range  = bufferElemCount * sizeof(uint32_t);
               bufferInfos[1].buffer = data.vkIndirectionBuffers[i];
               bufferInfos[1].offset = 0;
               bufferInfos[1].range  = bufferElemCount * sizeof(uint32_t);
        
               VkWriteDescriptorSet descriptorWrites[2] = {};
               descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
               descriptorWrites[0].dstSet          = data.vkIndirectDescriptorSets[i];
               descriptorWrites[0].dstBinding      = 0;
               descriptorWrites[0].dstArrayElement = 0;
               descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
               descriptorWrites[0].descriptorCount = 1;
               descriptorWrites[0].pBufferInfo     = &bufferInfos[0];
               descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
               descriptorWrites[1].dstSet          = data.vkIndirectDescriptorSets[i];
               descriptorWrites[1].dstBinding      = 1;
               descriptorWrites[1].dstArrayElement = 0;
               descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
               descriptorWrites[1].descriptorCount = 1;
               descriptorWrites[1].pBufferInfo     = &bufferInfos[1];
        
               vkUpdateDescriptorSets(vkDevice, 2, descriptorWrites, 0, nullptr);
          }
     }

     // Graphics descriptor sets.
     {
          // Allocate the descriptor sets.
          const std::vector layouts(MAX_FRAMES_IN_FLIGHT, modelsArray.vkGraphicsDescriptorSetLayout);
          VkDescriptorSetAllocateInfo allocInfo{};
          allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
          allocInfo.descriptorPool     = modelsArray.vkGraphicsDescriptorPool;
          allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
          allocInfo.pSetLayouts        = layouts.data();
          if (vkAllocateDescriptorSets(vkDevice, &allocInfo, data.vkTransformDescriptorSets) != VK_SUCCESS) {
               LogError(LogType::Vulkan, "Failed to allocate descriptor sets.");
               throw std::runtime_error("VULKAN_DESCRIPTOR_SET_ALLOCATION_ERROR");
          }

          // Populate the descriptor sets.
          for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
          {
               VkDescriptorBufferInfo mvpBufferInfo{};
               mvpBufferInfo.buffer = data.vkTransformBuffers[i];
               mvpBufferInfo.offset = 0;
               mvpBufferInfo.range  = bufferElemCount * sizeof(Mat4);
        
               VkWriteDescriptorSet descriptorWrite{};
               descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
               descriptorWrite.dstSet          = data.vkTransformDescriptorSets[i];
               descriptorWrite.dstBinding      = 0;
               descriptorWrite.dstArrayElement = 0;
               descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
               descriptorWrite.descriptorCount = 1;
               descriptorWrite.pBufferInfo     = &mvpBufferInfo;
        
               vkUpdateDescriptorSets(vkDevice, 1, &descriptorWrite, 0, nullptr);
          }
     }
    
    return data;
}
