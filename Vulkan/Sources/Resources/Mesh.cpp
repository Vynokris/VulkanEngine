#include "Resources/Mesh.h"
#include "Core/Application.h"
#include "Core/Renderer.h"
#include "Core/VulkanUtils.h"
#include <vulkan/vulkan.h>
#include <array>
using namespace Core;
using namespace Resources;
using namespace VkUtils;

Mesh::Mesh(Mesh&& other) noexcept
    : name(std::move(other.name)), material(other.material), parentModel(other.parentModel),
      vertices(std::move(other.vertices)), indices(std::move(other.indices)),
      vkVertexBuffer(other.vkVertexBuffer), vkVertexBufferMemory(other.vkVertexBufferMemory),
      vkIndexBuffer (other.vkIndexBuffer ), vkIndexBufferMemory (other.vkIndexBufferMemory )
{
    other.material       = nullptr;
    other.vkVertexBuffer = nullptr; other.vkVertexBufferMemory = nullptr;
    other.vkIndexBuffer  = nullptr; other.vkIndexBufferMemory  = nullptr;
}

Mesh::~Mesh()
{
    const VkDevice vkDevice = Application::Get()->GetRenderer()->GetVkDevice();
    if (vkIndexBuffer       ) vkDestroyBuffer(vkDevice, vkIndexBuffer,        nullptr);
    if (vkIndexBufferMemory ) vkFreeMemory   (vkDevice, vkIndexBufferMemory,  nullptr);
    if (vkVertexBuffer      ) vkDestroyBuffer(vkDevice, vkVertexBuffer,       nullptr);
    if (vkVertexBufferMemory) vkFreeMemory   (vkDevice, vkVertexBufferMemory, nullptr);
    vkIndexBuffer        = nullptr;
    vkIndexBufferMemory  = nullptr;
    vkVertexBuffer       = nullptr;
    vkVertexBufferMemory = nullptr;
}

VkVertexInputBindingDescription Mesh::GetVertexBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding   = 0;
    bindingDescription.stride    = sizeof(Maths::TangentVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 5> Mesh::GetVertexAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};
    
    attributeDescriptions[0].binding  = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset   = offsetof(Maths::TangentVertex, pos);
    
    attributeDescriptions[1].binding  = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format   = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset   = offsetof(Maths::TangentVertex, uv);

    attributeDescriptions[2].binding  = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset   = offsetof(Maths::TangentVertex, normal);
    
    attributeDescriptions[3].binding  = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[3].offset   = offsetof(Maths::TangentVertex, tangent);

    attributeDescriptions[4].binding  = 0;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[4].offset   = offsetof(Maths::TangentVertex, bitangent);

    return attributeDescriptions;
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
