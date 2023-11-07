#include "Core/Application.h"
#include "Resources/Model.h"
#include "Resources/Camera.h"
#include "Resources/Mesh.h"
#include "Maths/Vertex.h"
#include <vulkan/vulkan.h>
using namespace Core;
using namespace VkUtils;
using namespace Resources;
using namespace Maths;

Model::Model(std::string _name, Transform _transform)
     : name(std::move(_name)), transform(std::move(_transform))
{
     transform.SetRotation({ 0, 1, 0, 0 });
     CreateMvpBuffers();
     CreateDescriptorSets();
}

Model& Model::operator=(Model&& other) noexcept
{
     name      = other.name;                 other.name = "";
     meshes    = std::move(other.meshes);    other.meshes.clear();
     transform = std::move(other.transform); other.transform = {};
     memmove(vkDescriptorSets,   other.vkDescriptorSets,   sizeof(vkDescriptorSets));
     memmove(vkMvpBuffers,       other.vkMvpBuffers,       sizeof(vkMvpBuffers));
     memmove(vkMvpBuffersMemory, other.vkMvpBuffersMemory, sizeof(vkMvpBuffersMemory));
     memmove(vkMvpBuffersMapped, other.vkMvpBuffersMapped, sizeof(vkMvpBuffersMapped));
     memset(other.vkDescriptorSets,   NULL, sizeof(vkDescriptorSets));
     memset(other.vkMvpBuffers,       NULL, sizeof(vkMvpBuffers));
     memset(other.vkMvpBuffersMemory, NULL, sizeof(vkMvpBuffersMemory));
     memset(other.vkMvpBuffersMapped, NULL, sizeof(vkMvpBuffersMapped));
     return *this;
}

Model::~Model()
{
     const VkDevice vkDevice = Application::Get()->GetRenderer()->GetVkDevice();

     for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
          if (vkMvpBuffers[i])       vkDestroyBuffer(vkDevice, vkMvpBuffers[i],       nullptr);
          if (vkMvpBuffersMemory[i]) vkFreeMemory   (vkDevice, vkMvpBuffersMemory[i], nullptr);
     }
}

void Model::UpdateMvpBuffer(const Camera* camera, const uint32_t& currentFrame) const
{
     // Copy the matrices to buffer memory.
     const MvpBuffer mvp = { transform.GetLocalMat(), transform.GetLocalMat() * camera->GetViewMat() * camera->GetProjMat() };
     memcpy(vkMvpBuffersMapped[currentFrame], &mvp, sizeof(mvp));
}

void Model::CreateDescriptorLayoutAndPool(const VkDevice& vkDevice)
{
     if (vkDescriptorSetLayout && vkDescriptorPool) return;
     
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
     if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &vkDescriptorSetLayout) != VK_SUCCESS) {
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
     if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &vkDescriptorPool) != VK_SUCCESS) {
          LogError(LogType::Vulkan, "Failed to create descriptor pool.");
          throw std::runtime_error("VULKAN_DESCRIPTOR_POOL_ERROR");
     }
}

void Model::DestroyDescriptorLayoutAndPool(const VkDevice& vkDevice)
{
     if (vkDescriptorSetLayout) vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, nullptr);
     if (vkDescriptorPool)      vkDestroyDescriptorPool     (vkDevice, vkDescriptorPool,      nullptr);
}

VkDescriptorSet Model::GetVkDescriptorSet(const uint32_t& currentFrame) const
{
     if (currentFrame > MAX_FRAMES_IN_FLIGHT)
          return nullptr;
     return vkDescriptorSets[currentFrame];
}

void Model::CreateMvpBuffers()
{
     // Get necessary vulkan variables.
     const Renderer*        renderer         = Application::Get()->GetRenderer();
     const VkDevice         vkDevice         = renderer->GetVkDevice();
     const VkPhysicalDevice vkPhysicalDevice = renderer->GetVkPhysicalDevice();

     // Create the buffers.
     for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
     {
          CreateBuffer(vkDevice, vkPhysicalDevice, sizeof(MvpBuffer),
                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       vkMvpBuffers[i], vkMvpBuffersMemory[i]);

          vkMapMemory(vkDevice, vkMvpBuffersMemory[i], 0, sizeof(MvpBuffer), 0, &vkMvpBuffersMapped[i]);
     }
}

void Model::CreateDescriptorSets()
{
     // Get necessary vulkan variables.
     const Renderer* renderer = Application::Get()->GetRenderer();
     const VkDevice  vkDevice = renderer->GetVkDevice();
    
     // Allocate the descriptor sets.
     const std::vector layouts(MAX_FRAMES_IN_FLIGHT, vkDescriptorSetLayout);
     VkDescriptorSetAllocateInfo allocInfo{};
     allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
     allocInfo.descriptorPool     = vkDescriptorPool;
     allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
     allocInfo.pSetLayouts        = layouts.data();
     if (vkAllocateDescriptorSets(vkDevice, &allocInfo, vkDescriptorSets) != VK_SUCCESS) {
          LogError(LogType::Vulkan, "Failed to allocate descriptor sets.");
          throw std::runtime_error("VULKAN_DESCRIPTOR_SET_ALLOCATION_ERROR");
     }

     // Populate the descriptor sets.
     for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
     {
          VkDescriptorBufferInfo mvpBufferInfo{};
          mvpBufferInfo.buffer = vkMvpBuffers[i];
          mvpBufferInfo.offset = 0;
          mvpBufferInfo.range  = sizeof(MvpBuffer);
        
          VkWriteDescriptorSet descriptorWrite{};
          descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          descriptorWrite.dstSet          = vkDescriptorSets[i];
          descriptorWrite.dstBinding      = 0;
          descriptorWrite.dstArrayElement = 0;
          descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
          descriptorWrite.descriptorCount = 1;
          descriptorWrite.pBufferInfo     = &mvpBufferInfo;
        
          vkUpdateDescriptorSets(vkDevice, 1, &descriptorWrite, 0, nullptr);
     }
}
