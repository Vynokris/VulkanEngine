#include "Resources/Mesh.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "Core/GpuDataManager.h"
#include "Core/GraphicsUtils.h"
#include <vulkan/vulkan.h>
#include <array>
using namespace Core;
using namespace Resources;
using namespace GraphicsUtils;

Mesh::Mesh(Mesh&& other) noexcept
    : UniqueID(std::move(other)), name(std::move(other.name)), material(other.material),
      parentModel(other.parentModel), vertices(std::move(other.vertices)), indices(std::move(other.indices))
{
    other.material = nullptr;
}

Mesh::~Mesh()
{
    Application::Get()->GetGpuData()->DestroyData(*this);
}

void Mesh::FinalizeLoading()
{
    Application::Get()->GetGpuData()->CreateData(*this);
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


template<> const GpuData<Mesh>& GpuDataManager::CreateData(const Mesh& resource)
{
    if (resource.GetID() == UniqueID::unassigned) {
        LogError(LogType::Resources, "Can't create GPU data from unassigned resource.");
        throw std::runtime_error("RESOURCE_UNASSIGNED_ERROR");
    }
    GpuData<Mesh>& data = meshes.emplace(std::make_pair(resource.GetID(), GpuData<Mesh>())).first->second;

    // Get necessary vulkan resources.
    const VkDevice         vkDevice         = renderer->GetVkDevice();
    const VkPhysicalDevice vkPhysicalDevice = renderer->GetVkPhysicalDevice();
    const VkCommandPool    vkCommandPool    = renderer->GetVkCommandPool();
    const VkQueue          vkGraphicsQueue  = renderer->GetVkGraphicsQueue();

    // Create vertex buffer.
    {
        const std::vector<Maths::TangentVertex>& vertices = resource.GetVertices();
        
        // Create a temporary staging buffer.
        const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        VkBuffer       stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);

        // Map the buffer's GPU memory to CPU memory, and write vertex info to it.
        void* memMap;
        vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &memMap);
        memcpy(memMap, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(vkDevice, stagingBufferMemory);

        // Create the vertex buffer.
        CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     data.vkVertexBuffer, data.vkVertexBufferMemory);

        // Copy the staging buffer to the vertex buffer.
        CopyBuffer(vkDevice, vkCommandPool, vkGraphicsQueue, stagingBuffer, data.vkVertexBuffer, bufferSize);

        // De-allocate the staging buffer.
        vkDestroyBuffer(vkDevice, stagingBuffer,       nullptr);
        vkFreeMemory   (vkDevice, stagingBufferMemory, nullptr);
    }

    // Create index buffer.
    {
        const std::vector<uint32_t>& indices = resource.GetIndices();
        
        // Create a temporary staging buffer.
        const VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
        VkBuffer       stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);

        // Map the buffer's GPU memory to CPU memory, and write vertex info to it.
        void* memMap;
        vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &memMap);
        memcpy(memMap, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(vkDevice, stagingBufferMemory);

        // Create the vertex buffer.
        CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     data.vkIndexBuffer, data.vkIndexBufferMemory);

        // Copy the staging buffer to the vertex buffer.
        CopyBuffer(vkDevice, vkCommandPool, vkGraphicsQueue, stagingBuffer, data.vkIndexBuffer, bufferSize);

        // De-allocate the staging buffer.
        vkDestroyBuffer(vkDevice, stagingBuffer,       nullptr);
        vkFreeMemory   (vkDevice, stagingBufferMemory, nullptr);
    }
    
    return data;
}
