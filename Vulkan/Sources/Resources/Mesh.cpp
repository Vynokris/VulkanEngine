#include "Resources/Mesh.h"
#include "Core/Renderer.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <vulkan/vulkan.h>
#include <unordered_map>

#include "Core/Application.h"
using namespace Core;
using namespace Resources;

Mesh::Mesh(std::vector<Maths::TestVertex> _vertices, std::vector<uint32_t> _indices)
    : vertices(std::move(_vertices)), indices(std::move(_indices))
{
    CreateVertexBuffer();
    CreateIndexBuffer();
}

Mesh::Mesh(const char* filename)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename)) {
        std::cout << "ERROR (OBJ): Unable to load OBJ " << filename << std::endl << err << std::endl;
        throw std::runtime_error("OBJ_LOAD_ERROR");
    }

    std::unordered_map<Maths::TestVertex, uint32_t> uniqueVertices{};
    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Maths::TestVertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.f, 1.f, 1.f };

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = (uint32_t)vertices.size();
                vertices.push_back(vertex);
            }
            indices.push_back(uniqueVertices[vertex]);
        }
    }

    CreateVertexBuffer();
    CreateIndexBuffer();
}

Mesh::~Mesh()
{
    const VkDevice vkDevice = Application::Get()->GetRenderer()->GetVkDevice();
    if (vkIndexBuffer        != nullptr) vkDestroyBuffer(vkDevice, vkIndexBuffer,        nullptr);
    if (vkIndexBufferMemory  != nullptr) vkFreeMemory   (vkDevice, vkIndexBufferMemory,  nullptr);
    if (vkVertexBuffer       != nullptr) vkDestroyBuffer(vkDevice, vkVertexBuffer,       nullptr);
    if (vkVertexBufferMemory != nullptr) vkFreeMemory   (vkDevice, vkVertexBufferMemory, nullptr);
    vkIndexBuffer        = nullptr;
    vkIndexBufferMemory  = nullptr;
    vkVertexBuffer       = nullptr;
    vkVertexBufferMemory = nullptr;
}

void Mesh::CreateVertexBuffer()
{
    const Renderer*        renderer         = Application::Get()->GetRenderer();
    const VkDevice         vkDevice         = renderer->GetVkDevice();
    const VkPhysicalDevice vkPhysicalDevice = renderer->GetVkPhysicalDevice();
    const VkCommandPool    vkCommandPool    = renderer->GetVkCommandPool();
    const VkQueue          vkGraphicsQueue  = renderer->GetVkGraphicsQueue();
    
    // Create a temporary staging buffer.
    const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VulkanUtils::CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    // Map the buffer's GPU memory to CPU memory, and write vertex info to it.
    void* data;
    vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(vkDevice, stagingBufferMemory);

    // Create the vertex buffer.
    VulkanUtils::CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 vkVertexBuffer, vkVertexBufferMemory);

    // Copy the staging buffer to the vertex buffer.
    VulkanUtils::CopyBuffer(vkDevice, vkCommandPool, vkGraphicsQueue, stagingBuffer, vkVertexBuffer, bufferSize);

    // De-allocate the staging buffer.
    vkDestroyBuffer(vkDevice, stagingBuffer,       nullptr);
    vkFreeMemory   (vkDevice, stagingBufferMemory, nullptr);
}

void Mesh::CreateIndexBuffer()
{
    const Renderer*        renderer         = Application::Get()->GetRenderer();
    const VkDevice         vkDevice         = renderer->GetVkDevice();
    const VkPhysicalDevice vkPhysicalDevice = renderer->GetVkPhysicalDevice();
    const VkCommandPool    vkCommandPool    = renderer->GetVkCommandPool();
    const VkQueue          vkGraphicsQueue  = renderer->GetVkGraphicsQueue();
    
    // Create a temporary staging buffer.
    const VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VulkanUtils::CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              stagingBuffer, stagingBufferMemory);

    // Map the buffer's GPU memory to CPU memory, and write vertex info to it.
    void* data;
    vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(vkDevice, stagingBufferMemory);

    // Create the vertex buffer.
    VulkanUtils::CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              vkIndexBuffer, vkIndexBufferMemory);

    // Copy the staging buffer to the vertex buffer.
    VulkanUtils::CopyBuffer(vkDevice, vkCommandPool, vkGraphicsQueue, stagingBuffer, vkIndexBuffer, bufferSize);

    // De-allocate the staging buffer.
    vkDestroyBuffer(vkDevice, stagingBuffer,       nullptr);
    vkFreeMemory   (vkDevice, stagingBufferMemory, nullptr);
}
