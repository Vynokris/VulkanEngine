#include "Core/Application.h"
#include "Resources/Model.h"
#include "Resources/Texture.h"
#include <vulkan/vulkan.h>
#include <array>

#include "Resources/Camera.h"
using namespace Core;
using namespace Resources;

Model::Model(Mesh* _mesh, Texture* _texture, Maths::Transform _transform)
     : mesh(_mesh), texture(_texture), transform(std::move(_transform))
{
     CreateUniformBuffers();
     CreateDescriptorPool();
     CreateDescriptorSets();
}

Model::~Model()
{
     const VkDevice vkDevice = Application::Get()->GetRenderer()->GetVkDevice();
     
     for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
          vkDestroyBuffer(vkDevice, vkUniformBuffers[i],       nullptr);
          vkFreeMemory   (vkDevice, vkUniformBuffersMemory[i], nullptr);
     }
     vkDestroyDescriptorPool(vkDevice, vkDescriptorPool, nullptr);
}

void Model::UpdateUniformBuffer(const Camera* camera, const uint32_t& currentFrame) const
{
     // Copy the matrices to buffer memory.
     const UniformBufferObject ubo = { transform.GetWorldMat(), camera->GetViewMat(), camera->GetProjMat() };
     memcpy(vkUniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

void Model::CreateUniformBuffers()
{
     // Get necessary vulkan variables.
     const Renderer*        renderer         = Application::Get()->GetRenderer();
     const VkDevice         vkDevice         = renderer->GetVkDevice();
     const VkPhysicalDevice vkPhysicalDevice = renderer->GetVkPhysicalDevice();

     // Find the size of the buffers and resize the buffer arrays.
     const VkDeviceSize bufferSize = sizeof(UniformBufferObject);
     vkUniformBuffers      .resize(MAX_FRAMES_IN_FLIGHT);
     vkUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
     vkUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

     // Create the buffers.
     for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
     {
          VulkanUtils::CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                    vkUniformBuffers[i], vkUniformBuffersMemory[i]);

          vkMapMemory(vkDevice, vkUniformBuffersMemory[i], 0, bufferSize, 0, &vkUniformBuffersMapped[i]);
     }
}

void Model::CreateDescriptorPool()
{
     // Set the type and number of descriptors.
     std::array<VkDescriptorPoolSize, 2> poolSizes{};
     poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
     poolSizes[0].descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;
     poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
     poolSizes[1].descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;

     // Create the descriptor pool.
     VkDescriptorPoolCreateInfo poolInfo{};
     poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
     poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
     poolInfo.pPoolSizes    = poolSizes.data();
     poolInfo.maxSets       = (uint32_t)MAX_FRAMES_IN_FLIGHT;
     if (vkCreateDescriptorPool(Application::Get()->GetRenderer()->GetVkDevice(), &poolInfo, nullptr, &vkDescriptorPool) != VK_SUCCESS) {
          std::cout << "ERROR (Vulkan): Failed to create descriptor pool." << std::endl;
          throw std::runtime_error("VULKAN_DESCRIPTOR_POOL_ERROR");
     }
}

void Model::CreateDescriptorSets()
{
     // Get necessary vulkan variables.
     const Renderer* renderer = Application::Get()->GetRenderer();
     const VkDevice  vkDevice = renderer->GetVkDevice();
     
     // Create the descriptor sets.
    const std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, renderer->GetVkDescriptorSetLayout());
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = vkDescriptorPool;
    allocInfo.descriptorSetCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts        = layouts.data();
    vkDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(vkDevice, &allocInfo, vkDescriptorSets.data()) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to allocate descriptor sets." << std::endl;
        throw std::runtime_error("VULKAN_DESCRIPTOR_SETS_ERROR");
    }

    // Populate the descriptor sets.
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = vkUniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range  = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView   = texture->GetVkImageView();
        imageInfo.sampler     = renderer->GetVkTextureSampler();

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet           = vkDescriptorSets[i];
        descriptorWrites[0].dstBinding       = 0;
        descriptorWrites[0].dstArrayElement  = 0;
        descriptorWrites[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount  = 1;
        descriptorWrites[0].pBufferInfo      = &bufferInfo;

        descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet          = vkDescriptorSets[i];
        descriptorWrites[1].dstBinding      = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo      = &imageInfo;
        
        vkUpdateDescriptorSets(vkDevice, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}
