#include "Resources/Mesh.h"
#include "Resources/Texture.h"
#include "Core/Application.h"
#include "Core/Renderer.h"
#include "Core/VulkanUtils.h"
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <array>

#include "Resources/Model.h"
using namespace Core;
using namespace Resources;
using namespace VulkanUtils;

Mesh::Mesh(Mesh&& other) noexcept
    : name(std::move(other.name)), material(std::move(other.material)), loadingFinalized(other.loadingFinalized), parentModel(other.parentModel),
      vertices(std::move(other.vertices)), indices(std::move(other.indices)),
      vkDescriptorPool(other.vkDescriptorPool), vkDescriptorSets(std::move(other.vkDescriptorSets)),
      vkVertexBuffer  (other.vkVertexBuffer  ), vkVertexBufferMemory(other.vkVertexBufferMemory),
      vkIndexBuffer   (other.vkIndexBuffer   ), vkIndexBufferMemory (other.vkIndexBufferMemory )
{
    other.vkDescriptorPool = nullptr; other.vkDescriptorSets.clear();
    other.vkVertexBuffer   = nullptr; other.vkVertexBufferMemory = nullptr;
    other.vkIndexBuffer    = nullptr; other.vkIndexBufferMemory  = nullptr;
}

Mesh::~Mesh()
{
    const VkDevice vkDevice = Application::Get()->GetRenderer()->GetVkDevice();
    if (vkIndexBuffer       ) vkDestroyBuffer        (vkDevice, vkIndexBuffer,        nullptr);
    if (vkIndexBufferMemory ) vkFreeMemory           (vkDevice, vkIndexBufferMemory,  nullptr);
    if (vkVertexBuffer      ) vkDestroyBuffer        (vkDevice, vkVertexBuffer,       nullptr);
    if (vkVertexBufferMemory) vkFreeMemory           (vkDevice, vkVertexBufferMemory, nullptr);
    if (vkDescriptorPool    ) vkDestroyDescriptorPool(vkDevice, vkDescriptorPool,     nullptr);
    vkIndexBuffer        = nullptr;
    vkIndexBufferMemory  = nullptr;
    vkVertexBuffer       = nullptr;
    vkVertexBufferMemory = nullptr;
}

void Mesh::CreateVertexBuffers()
{
    const Renderer*        renderer         = Application::Get()->GetRenderer();
    const VkDevice         vkDevice         = renderer->GetVkDevice();
    const VkPhysicalDevice vkPhysicalDevice = renderer->GetVkPhysicalDevice();
    const VkCommandPool    vkCommandPool    = renderer->GetVkCommandPool();
    const VkQueue          vkGraphicsQueue  = renderer->GetVkGraphicsQueue();

    // TODO: Make a method to create a buffer automatically.

    // Create vertex buffer.
    {
        // Create a temporary staging buffer.
        const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        VkBuffer       stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);

        // Map the buffer's GPU memory to CPU memory, and write vertex info to it.
        void* data;
        vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(vkDevice, stagingBufferMemory);

        // Create the vertex buffer.
        CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     vkVertexBuffer, vkVertexBufferMemory);

        // Copy the staging buffer to the vertex buffer.
        CopyBuffer(vkDevice, vkCommandPool, vkGraphicsQueue, stagingBuffer, vkVertexBuffer, bufferSize);

        // De-allocate the staging buffer.
        vkDestroyBuffer(vkDevice, stagingBuffer,       nullptr);
        vkFreeMemory   (vkDevice, stagingBufferMemory, nullptr);
    }

    // Create index buffer.
    {
        // Create a temporary staging buffer.
        const VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
        VkBuffer       stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);

        // Map the buffer's GPU memory to CPU memory, and write vertex info to it.
        void* data;
        vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(vkDevice, stagingBufferMemory);

        // Create the vertex buffer.
        CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     vkIndexBuffer, vkIndexBufferMemory);

        // Copy the staging buffer to the vertex buffer.
        CopyBuffer(vkDevice, vkCommandPool, vkGraphicsQueue, stagingBuffer, vkIndexBuffer, bufferSize);

        // De-allocate the staging buffer.
        vkDestroyBuffer(vkDevice, stagingBuffer,       nullptr);
        vkFreeMemory   (vkDevice, stagingBufferMemory, nullptr);
    }
}

void Mesh::CreateDescriptors()
{
    // Get necessary vulkan variables.
    const Renderer* renderer = Application::Get()->GetRenderer();
    const VkDevice  vkDevice = renderer->GetVkDevice();

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
        LogError(LogType::Vulkan, "Failed to create descriptor pool.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_POOL_ERROR");
    }
    
    // Create the descriptor sets.
    const std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, renderer->GetVkDescriptorSetLayout());
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = vkDescriptorPool;
    allocInfo.descriptorSetCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts        = layouts.data();
    vkDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(vkDevice, &allocInfo, vkDescriptorSets.data()) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to allocate descriptor sets.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_SETS_ERROR");
    }

    // Populate the descriptor sets.
    for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo mvpBufferInfo{};
        mvpBufferInfo.buffer = parentModel.vkMvpBuffers[i];
        mvpBufferInfo.offset = 0;
        mvpBufferInfo.range  = sizeof(Maths::MvpBuffer);
        
        VkDescriptorImageInfo imagesInfo[Material::textureTypesCount];
        for (size_t j = 0; j < Material::textureTypesCount; j++)
        {
            const Texture* texture    = material.textures[MaterialTextureType::Albedo+j];
            imagesInfo[j].imageView   = texture ? texture->GetVkImageView() : VK_NULL_HANDLE;
            imagesInfo[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imagesInfo[j].sampler     = renderer->GetVkTextureSampler();
        }

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet          = vkDescriptorSets[i];
        descriptorWrites[0].dstBinding      = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo     = &mvpBufferInfo;
        descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet          = vkDescriptorSets[i];
        descriptorWrites[1].dstBinding      = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = Material::textureTypesCount;
        descriptorWrites[1].pImageInfo      = imagesInfo;
        
        vkUpdateDescriptorSets(vkDevice, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}
