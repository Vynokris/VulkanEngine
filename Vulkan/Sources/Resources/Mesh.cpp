#include "Resources/Mesh.h"
#include "Core/Application.h"
#include "Core/Renderer.h"
#include "Core/VulkanUtils.h"
#include <vulkan/vulkan.h>
#include <unordered_map>
using namespace Core;
using namespace Resources;
using namespace VulkanUtils;

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
    if (vkIndexBuffer       ) vkDestroyBuffer        (vkDevice, vkIndexBuffer,        nullptr);
    if (vkIndexBufferMemory ) vkFreeMemory           (vkDevice, vkIndexBufferMemory,  nullptr);
    if (vkVertexBuffer      ) vkDestroyBuffer        (vkDevice, vkVertexBuffer,       nullptr);
    if (vkVertexBufferMemory) vkFreeMemory           (vkDevice, vkVertexBufferMemory, nullptr);
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
