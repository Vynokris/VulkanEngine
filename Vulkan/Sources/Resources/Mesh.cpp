#include "Resources/Mesh.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "Core/GpuDataManager.h"
#include "Core/Engine.h"
#include "Core/GraphicsUtils.h"
#include <vulkan/vulkan.h>
#include <array>
using namespace Core;
using namespace Resources;
using namespace GraphicsUtils;

Mesh::Mesh(Mesh&& other) noexcept
    : UniqueID(std::move(other)), name(std::move(other.name)), material(other.material), parentModel(other.parentModel),
      vertices(std::move(other.vertices)), indices(std::move(other.indices)), sections(std::move(other.sections))
{
    other.material = nullptr;
}

Mesh::~Mesh()
{
    Application::Get()->GetGpuData()->DestroyData(*this);
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
    attributeDescriptions[4].offset   = offsetof(Maths::TangentVertex, binormal);

    return attributeDescriptions;
}

void Mesh::StartNewSection()
{
    if (sections.empty())
    {
        sections.push_back({ 0u, 0u, GetVertexCount(), GetIndexCount() });
    }
    else
    {
        const Section& prev = sections.back();
        const uint32_t vtxStart = prev.vtxOffset + prev.vtxCount;
        const uint32_t idxStart = prev.idxOffset + prev.idxCount;
        const uint32_t vtxCount = GetVertexCount() - vtxStart;
        const uint32_t idxCount = GetIndexCount() - idxStart;
        sections.push_back({ vtxStart, idxStart, vtxCount, idxCount });
    }
}

void Mesh::FinalizeLoading()
{
    StartNewSection();
    Application::Get()->GetGpuData()->CreateData(*this);
}


template<> const GpuArray<Mesh>& GpuDataManager::CreateArray()
{
    if (meshesArray.vkComputeDescriptorSetLayout  && meshesArray.vkDescriptorPool)
        return meshesArray;

    // Get the necessary vulkan resources.
    const VkDevice vkDevice = renderer->GetVkDevice();

    // Compute layout.
    {
        // Set the binding of the draw indirect and indirection offsets buffers.
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
        if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &meshesArray.vkComputeDescriptorSetLayout) != VK_SUCCESS) {
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
        if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &meshesArray.vkDescriptorPool) != VK_SUCCESS) {
             LogError(LogType::Vulkan, "Failed to create descriptor pool.");
             throw std::runtime_error("VULKAN_DESCRIPTOR_POOL_ERROR");
        }
    }
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

        // Map the buffer's GPU memory to CPU memory, and write data to it.
        void* memMap;
        vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &memMap);
        memcpy(memMap, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(vkDevice, stagingBufferMemory);

        // Create the final buffer.
        CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     data.vkVertexBuffer, data.vkVertexBufferMemory);

        // Copy the staging buffer to the final buffer.
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

        // Map the buffer's GPU memory to CPU memory, and write data to it.
        void* memMap;
        vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &memMap);
        memcpy(memMap, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(vkDevice, stagingBufferMemory);

        // Create the final buffer.
        CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     data.vkIndexBuffer, data.vkIndexBufferMemory);

        // Copy the staging buffer to the final buffer.
        CopyBuffer(vkDevice, vkCommandPool, vkGraphicsQueue, stagingBuffer, data.vkIndexBuffer, bufferSize);

        // De-allocate the staging buffer.
        vkDestroyBuffer(vkDevice, stagingBuffer,       nullptr);
        vkFreeMemory   (vkDevice, stagingBufferMemory, nullptr);
    }

    // Create indirect rendering data.
    if (resource.HasSections())
    {
        const std::vector<Mesh::Section>& sections = resource.GetSections();

        const VkDeviceSize bufferElemCount = sections.size();
        const VkDeviceSize drawIndirectBufferSize = bufferElemCount * sizeof(VkDrawIndexedIndirectCommand);
        
        // Create a temporary staging buffer.
        VkBuffer       stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        CreateBuffer(vkDevice, vkPhysicalDevice, drawIndirectBufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);

        // Map the buffer's GPU memory to CPU memory, and write data to it.
        VkDrawIndexedIndirectCommand* memMap;
        vkMapMemory(vkDevice, stagingBufferMemory, 0, drawIndirectBufferSize, 0, (void**)&memMap);
        for (size_t i = 0; i < sections.size(); i++)
        {
            const Mesh::Section&          section     = sections[i];
            VkDrawIndexedIndirectCommand& indirectCmd = memMap[i];
            indirectCmd.indexCount    = section.idxCount;
            indirectCmd.instanceCount = 0;
            indirectCmd.firstIndex    = section.idxOffset;
            indirectCmd.vertexOffset  = 0;
            indirectCmd.firstInstance = 0;
        }
        vkUnmapMemory(vkDevice, stagingBufferMemory);

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            // Create the final buffers.
            CreateBuffer(vkDevice, vkPhysicalDevice, drawIndirectBufferSize,
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                         data.vkDrawIndirectBuffers[i], data.vkDrawIndirectBuffersMemory[i]);

            // Copy the staging buffer to the final buffers.
            CopyBuffer(vkDevice, vkCommandPool, vkGraphicsQueue, stagingBuffer, data.vkDrawIndirectBuffers[i], drawIndirectBufferSize);
            
            // Create the indirection offsets buffers (written in compute).
            CreateBuffer(vkDevice, vkPhysicalDevice, bufferElemCount * sizeof(uint32_t) * 2,
                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                         data.vkIndirectionOffsetsBuffers[i], data.vkIndirectionOffsetsBuffersMemory[i]);
        }

        // De-allocate the staging buffer.
        vkDestroyBuffer(vkDevice, stagingBuffer,       nullptr);
        vkFreeMemory   (vkDevice, stagingBufferMemory, nullptr);
        
        // Allocate the descriptor sets.
        const std::vector layouts(MAX_FRAMES_IN_FLIGHT, meshesArray.vkComputeDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = meshesArray.vkDescriptorPool;
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
            bufferInfos[0].buffer = data.vkDrawIndirectBuffers[i];
            bufferInfos[0].range  = drawIndirectBufferSize;
            bufferInfos[0].offset = 0;
            bufferInfos[1].buffer = data.vkIndirectionOffsetsBuffers[i];
            bufferInfos[1].range  = bufferElemCount * sizeof(uint32_t);
            bufferInfos[1].offset = 0;

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
    
    return data;
}
