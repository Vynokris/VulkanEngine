#include "Core/GpuDataManager.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "Core/Engine.h"
#include "Resources/Texture.h"
#include "Resources/Material.h"
#include "Resources/Mesh.h"
#include "Resources/Model.h"
#include "Resources/Light.h"
#include <vulkan/vulkan_core.h>
using namespace Core;
using namespace Resources;
using namespace GraphicsUtils;

GpuDataManager::~GpuDataManager()
{
}

template<> const DataPool<Material>& GpuDataManager::CreatePool()
{
    if (materialPool.vkDescriptorSetLayout && materialPool.vkDescriptorPool) return materialPool;

    // Get the necessary vulkan resources.
    const VkDevice vkDevice = renderer->GetVkDevice();
     
    // Set the bindings of the material data and textures.
    VkDescriptorSetLayoutBinding layoutBindings[2] = {{},{}};
    layoutBindings[0].binding         = 0;
    layoutBindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBindings[1].binding         = 1;
    layoutBindings[1].descriptorCount = MaterialTextureType::COUNT;
    layoutBindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindings[1].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Create the descriptor set layout.
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings    = layoutBindings;
    if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &materialPool.vkDescriptorSetLayout) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create descriptor set layout.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_SET_LAYOUT_ERROR");
    }

    // Set the type and number of descriptors.
    VkDescriptorPoolSize poolSizes[2];
    poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = Engine::MAX_MATERIALS;
    poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = Engine::MAX_MATERIALS;

    // Create the descriptor pool.
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes    = poolSizes;
    poolInfo.maxSets       = Engine::MAX_MATERIALS;
    if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &materialPool.vkDescriptorPool) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create descriptor pool.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_POOL_ERROR");
    }

    return materialPool;
}

template<> const DataPool<Model>& GpuDataManager::CreatePool()
{
    if (modelPool.vkDescriptorSetLayout && modelPool.vkDescriptorPool) return modelPool;

    // Get the necessary vulkan resources.
    const VkDevice vkDevice = renderer->GetVkDevice();
     
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
    if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &modelPool.vkDescriptorSetLayout) != VK_SUCCESS) {
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
    if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &modelPool.vkDescriptorPool) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create descriptor pool.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_POOL_ERROR");
    }

    return modelPool;
}

template<> const DataPool<Light>& GpuDataManager::CreatePool()
{
    if (lightPool.vkDescriptorSetLayout && lightPool.vkDescriptorPool && lightPool.vkBuffer && lightPool.vkBufferMemory) return lightPool;

    // Get the necessary vulkan resources.
    const VkDevice         vkDevice         = renderer->GetVkDevice();
    const VkPhysicalDevice vkPhysicalDevice = renderer->GetVkPhysicalDevice();

    // Set the binding of the data buffer object.
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding         = 0;
    layoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Create the descriptor set layout.
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings    = &layoutBinding;
    if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &lightPool.vkDescriptorSetLayout) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create descriptor set layout.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_SET_LAYOUT_ERROR");
    }

    // Set the type and number of descriptors.
    VkDescriptorPoolSize poolSize{};
    poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;

    // Create the descriptor pool.
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = &poolSize;
    poolInfo.maxSets       = 1;
    if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &lightPool.vkDescriptorPool) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create descriptor pool.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_POOL_ERROR");
    }

    // Create the light data buffer.
    CreateBuffer(vkDevice, vkPhysicalDevice, sizeof(Light) * Engine::MAX_LIGHTS,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                 lightPool.vkBuffer, lightPool.vkBufferMemory);

    // Allocate the descriptor set.
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = lightPool.vkDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &lightPool.vkDescriptorSetLayout;
    if (vkAllocateDescriptorSets(vkDevice, &allocInfo, &lightPool.vkDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    return lightPool;
}

template<> const GpuData<Texture>& GpuDataManager::CreateData(const Texture& resource)
{
    GpuData<Texture>& data = textures.emplace(std::make_pair(resource.GetID(), GpuData<Texture>())).first->second;
    data.vkImageFormat = VK_FORMAT_R8G8B8A8_UNORM;

    // Get necessary vulkan resources.
    const VkDevice         device         = renderer->GetVkDevice();
    const VkPhysicalDevice physicalDevice = renderer->GetVkPhysicalDevice();
    const VkCommandPool    commandPool    = renderer->GetVkCommandPool();
    const VkQueue          graphicsQueue  = renderer->GetVkGraphicsQueue();

    // Get texture data.
    const int      width     = resource.GetWidth();
    const int      height    = resource.GetHeight();
    const uint32_t mipLevels = resource.GetMipLevels();
    const VkDeviceSize imageSize = (VkDeviceSize)(width * height) * 4;

    // Create a transfer buffer to send the pixels to the GPU.
    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(device, physicalDevice, imageSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    // Copy the pixels to the transfer buffer.
    void* mapMem;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &mapMem);
    memcpy(mapMem, resource.GetPixels(), (size_t)imageSize);
    vkUnmapMemory(device, stagingBufferMemory);

    // Create the Vulkan image.
    CreateImage(device, physicalDevice, width, height, mipLevels, VK_SAMPLE_COUNT_1_BIT, data.vkImageFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                data.vkImage, data.vkImageMemory);

    // Copy the transfer buffer to the vulkan image.
    TransitionImageLayout(device, commandPool, graphicsQueue, data.vkImage, data.vkImageFormat, mipLevels, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CopyBufferToImage    (device, commandPool, graphicsQueue, stagingBuffer, data.vkImage, (uint32_t)width, (uint32_t)height);
    resource.GenerateMipmaps();

    // Cleanup allocated structures.
    vkDestroyBuffer(device, stagingBuffer,       nullptr);
    vkFreeMemory   (device, stagingBufferMemory, nullptr);

    // Create the texture image view.
    CreateImageView(device, data.vkImage, data.vkImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, data.vkImageView);
    
    return data;
}

template<> const GpuData<Material>& GpuDataManager::CreateData(const Material& resource)
{
    GpuData<Material>& data = materials.emplace(std::make_pair(resource.GetID(), GpuData<Material>())).first->second;

    // Get necessary vulkan resources.
    const VkDevice         vkDevice         = renderer->GetVkDevice();
    const VkPhysicalDevice vkPhysicalDevice = renderer->GetVkPhysicalDevice();

    // Create a temporary staging buffer.
    constexpr VkDeviceSize bufferSize = sizeof(MaterialData);
    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    // Map the buffer's GPU memory to CPU memory, and write vertex info to it.
    const MaterialData materialData{ resource.albedo, resource.emissive, resource.metallic, resource.roughness, resource.alpha, resource.depthMultiplier, resource.depthLayerCount };
    void* memMapped;
    vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &memMapped);
    memcpy(memMapped, &materialData, (size_t)bufferSize);
    vkUnmapMemory(vkDevice, stagingBufferMemory);

    // Create the vertex buffer.
    CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 data.vkDataBuffer, data.vkDataBufferMemory);

    // Copy the staging buffer to the vertex buffer.
    CopyBuffer(vkDevice, renderer->GetVkCommandPool(), renderer->GetVkGraphicsQueue(), stagingBuffer, data.vkDataBuffer, bufferSize);

    // De-allocate the staging buffer.
    vkDestroyBuffer(vkDevice, stagingBuffer,       nullptr);
    vkFreeMemory   (vkDevice, stagingBufferMemory, nullptr);
    
    // Allocate the descriptor set.
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = materialPool.vkDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &materialPool.vkDescriptorSetLayout;
    if (vkAllocateDescriptorSets(vkDevice, &allocInfo, &data.vkDescriptorSet) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to allocate descriptor sets.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_SET_ALLOCATION_ERROR");
    }

    // Populate the descriptor set.
    VkDescriptorBufferInfo dataBufferInfo{};
    dataBufferInfo.buffer = data.vkDataBuffer;
    dataBufferInfo.offset = 0;
    dataBufferInfo.range  = sizeof(MaterialData);
    
    VkDescriptorImageInfo imagesInfo[MaterialTextureType::COUNT];
    for (size_t j = 0; j < MaterialTextureType::COUNT; j++)
    {
        const Texture* texture    = resource.textures[MaterialTextureType::Albedo+j];
        imagesInfo[j].imageView   = texture ? texture->GetVkImageView() : VK_NULL_HANDLE;
        imagesInfo[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imagesInfo[j].sampler     = renderer->GetVkTextureSampler();
    }
    
    VkWriteDescriptorSet descriptorWrites[2] = {{},{}};
    descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet          = data.vkDescriptorSet;
    descriptorWrites[0].dstBinding      = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo     = &dataBufferInfo;
    descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet          = data.vkDescriptorSet;
    descriptorWrites[1].dstBinding      = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = MaterialTextureType::COUNT;
    descriptorWrites[1].pImageInfo      = imagesInfo;
    
    vkUpdateDescriptorSets(vkDevice, 2, descriptorWrites, 0, nullptr);
    
    return data;
}

template<> const GpuData<Mesh>& GpuDataManager::CreateData(const Mesh& resource)
{
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

template<> const GpuData<Model>& GpuDataManager::CreateData(const Model& resource)
{
    GpuData<Model>& data = models.emplace(std::make_pair(resource.GetID(), GpuData<Model>())).first->second;

    // Get necessary vulkan resources.
    const VkDevice         vkDevice         = renderer->GetVkDevice();
    const VkPhysicalDevice vkPhysicalDevice = renderer->GetVkPhysicalDevice();

    // Create the buffers.
    constexpr VkDeviceSize mvpSize = sizeof(Maths::MvpBuffer);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        CreateBuffer(vkDevice, vkPhysicalDevice, mvpSize,
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     data.vkMvpBuffers[i], data.vkMvpBuffersMemory[i]);

        vkMapMemory(vkDevice, data.vkMvpBuffersMemory[i], 0, mvpSize, 0, &data.vkMvpBuffersMapped[i]);
    }

    // Allocate the descriptor sets.
    const std::vector layouts(MAX_FRAMES_IN_FLIGHT, modelPool.vkDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = modelPool.vkDescriptorPool;
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
        mvpBufferInfo.range  = mvpSize;
        
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet          = data.vkDescriptorSets[i];
        descriptorWrite.dstBinding      = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo     = &mvpBufferInfo;
        
        vkUpdateDescriptorSets(vkDevice, 1, &descriptorWrite, 0, nullptr);
    }
    
    return data;
}

template<> void GpuDataManager::DestroyPool<Material>()
{
    const VkDevice vkDevice = renderer->GetVkDevice();
    if (materialPool.vkDescriptorSetLayout) vkDestroyDescriptorSetLayout(vkDevice, materialPool.vkDescriptorSetLayout, nullptr);
    if (materialPool.vkDescriptorPool)      vkDestroyDescriptorPool     (vkDevice, materialPool.vkDescriptorPool,      nullptr);
}

template<> void GpuDataManager::DestroyPool<Model>()
{
    const VkDevice vkDevice = renderer->GetVkDevice();
    if (modelPool.vkDescriptorSetLayout) vkDestroyDescriptorSetLayout(vkDevice, modelPool.vkDescriptorSetLayout, nullptr);
    if (modelPool.vkDescriptorPool)      vkDestroyDescriptorPool     (vkDevice, modelPool.vkDescriptorPool,      nullptr);
}

template<> void GpuDataManager::DestroyPool<Light>()
{
    const VkDevice vkDevice = renderer->GetVkDevice();
    if (lightPool.vkDescriptorSetLayout) vkDestroyDescriptorSetLayout(vkDevice, lightPool.vkDescriptorSetLayout, nullptr);
    if (lightPool.vkDescriptorPool)      vkDestroyDescriptorPool     (vkDevice, lightPool.vkDescriptorPool,      nullptr);
    if (lightPool.vkBuffer)              vkDestroyBuffer(vkDevice, lightPool.vkBuffer,       nullptr);
    if (lightPool.vkBufferMemory)        vkFreeMemory   (vkDevice, lightPool.vkBufferMemory, nullptr);
}

template<> void GpuDataManager::DestroyData(const Texture& resource)
{
    const VkDevice vkDevice = renderer->GetVkDevice();
    const GpuData<Texture>& data = textures.at(resource.GetID());
    if (data.vkImage      ) vkDestroyImage    (vkDevice, data.vkImage,       nullptr);
    if (data.vkImageMemory) vkFreeMemory      (vkDevice, data.vkImageMemory, nullptr);
    if (data.vkImageView  ) vkDestroyImageView(vkDevice, data.vkImageView,   nullptr);
    textures.erase(resource.GetID());
}

template<> void GpuDataManager::DestroyData(const Material& resource)
{
    const VkDevice vkDevice = renderer->GetVkDevice();
    const GpuData<Material>& data = materials.at(resource.GetID());
    if (data.vkDataBuffer)       vkDestroyBuffer(vkDevice, data.vkDataBuffer,       nullptr);
    if (data.vkDataBufferMemory) vkFreeMemory   (vkDevice, data.vkDataBufferMemory, nullptr);
    materials.erase(resource.GetID());
}

template<> void GpuDataManager::DestroyData(const Mesh& resource)
{
    const VkDevice vkDevice = renderer->GetVkDevice();
    const GpuData<Mesh>& data = meshes.at(resource.GetID());
    if (data.vkIndexBuffer       ) vkDestroyBuffer(vkDevice, data.vkIndexBuffer,        nullptr);
    if (data.vkIndexBufferMemory ) vkFreeMemory   (vkDevice, data.vkIndexBufferMemory,  nullptr);
    if (data.vkVertexBuffer      ) vkDestroyBuffer(vkDevice, data.vkVertexBuffer,       nullptr);
    if (data.vkVertexBufferMemory) vkFreeMemory   (vkDevice, data.vkVertexBufferMemory, nullptr);
    materials.erase(resource.GetID());
}

template<> void GpuDataManager::DestroyData(const Model& resource)
{
    const VkDevice vkDevice = renderer->GetVkDevice();
    const GpuData<Model>& data = models.at(resource.GetID());
    for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (data.vkMvpBuffers[i])       vkDestroyBuffer(vkDevice, data.vkMvpBuffers[i],       nullptr);
        if (data.vkMvpBuffersMemory[i]) vkFreeMemory   (vkDevice, data.vkMvpBuffersMemory[i], nullptr);
    }
    materials.erase(resource.GetID());
}

template<> const DataPool<Material>& GpuDataManager::GetPool() const { return materialPool; }
template<> const DataPool<Model>&    GpuDataManager::GetPool() const { return modelPool; }
template<> const DataPool<Light>&    GpuDataManager::GetPool() const { return lightPool; }

template<> const GpuData<Texture>& GpuDataManager::GetData(const Texture& resource) const
{
    Assert(textures.count(resource.GetID()) > 0, "No texture with ID " + std::to_string(resource.GetID()));
    return textures.at(resource.GetID());
}

template<> const GpuData<Material>& GpuDataManager::GetData(const Material& resource) const
{
    Assert(textures.count(resource.GetID()) > 0, "No material with ID " + std::to_string(resource.GetID()));
    return materials.at(resource.GetID());
}

template<> const GpuData<Mesh>& GpuDataManager::GetData(const Mesh& resource) const
{
    Assert(textures.count(resource.GetID()) > 0, "No mesh with ID " + std::to_string(resource.GetID()));
    return meshes.at(resource.GetID());
}

template<> const GpuData<Model>& GpuDataManager::GetData(const Model& resource) const
{
    Assert(textures.count(resource.GetID()) > 0, "No model with ID " + std::to_string(resource.GetID()));
    return models.at(resource.GetID());
}