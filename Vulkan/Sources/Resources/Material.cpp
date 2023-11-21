#include "Resources/Material.h"
#include "Core/Application.h"
#include <vulkan/vulkan_core.h>

#include "Maths/Vertex.h"
#include "Resources/Texture.h"
using namespace Core;
using namespace Maths;
using namespace Resources;
using namespace VkUtils;

Material::Material(const RGB& _albedo, const RGB& _emissive, const float& _metallic, const float& _roughness, const float& _alpha,
                 Texture* albedoTexture, Texture* emissiveTexture, Texture* metallicMap, Texture* roughnessMap, Texture* aoMap, Texture* alphaMap, Texture* normalMap)
    : albedo(_albedo), emissive(_emissive), metallic(_metallic), roughness(_roughness), alpha(_alpha)
{
    textures[MaterialTextureType::Albedo    ] = albedoTexture;
    textures[MaterialTextureType::Emissive  ] = emissiveTexture;
    textures[MaterialTextureType::Metallic  ] = metallicMap;
    textures[MaterialTextureType::Roughness ] = roughnessMap;
    textures[MaterialTextureType::AOcclusion] = aoMap;
    textures[MaterialTextureType::Alpha     ] = alphaMap;
    textures[MaterialTextureType::Normal    ] = normalMap;
}

Material& Material::operator=(Material&& other) noexcept
{
    name      = other.name;      other.name       = "";
    albedo    = other.albedo;    other.albedo     = 0;
    emissive  = other.emissive;  other.emissive   = 0;
    metallic  = other.metallic ; other.metallic   = 0;
    roughness = other.roughness; other.roughness  = 0;
    alpha     = other.alpha;     other.alpha      = 0;
    for (size_t i = 0; i < MaterialTextureType::COUNT; i++) {
        textures[i] = other.textures[i];
        other.textures[i] = nullptr;
    }
    vkDescriptorSet    = other.vkDescriptorSet;    other.vkDescriptorSet    = nullptr;
    vkDataBuffer       = other.vkDataBuffer;       other.vkDataBuffer       = nullptr;
    vkDataBufferMemory = other.vkDataBufferMemory; other.vkDataBufferMemory = nullptr;
    return *this;
}

Material::~Material()
{
    const VkDevice vkDevice = Application::Get()->GetRenderer()->GetVkDevice();
    if (vkDataBuffer)       vkDestroyBuffer(vkDevice, vkDataBuffer,       nullptr);
    if (vkDataBufferMemory) vkFreeMemory   (vkDevice, vkDataBufferMemory, nullptr);
}

void Material::SetParams(const RGB& _albedo, const RGB& _emissive, const float& _metallic, const float& _roughness, const float& _alpha)
{
    albedo    = _albedo;
    emissive  = _emissive;
    metallic  = _metallic;
    roughness = _roughness;
    alpha     = _alpha;
}

void Material::FinalizeLoading()
{
    if (IsLoadingFinalized()) return;
    
    // Get necessary vulkan variables.
    const Renderer*        renderer         = Application::Get()->GetRenderer();
    const VkDevice         vkDevice         = renderer->GetVkDevice();
    const VkPhysicalDevice vkPhysicalDevice = renderer->GetVkPhysicalDevice();

    // Create a temporary staging buffer.
    const VkDeviceSize bufferSize = sizeof(MaterialData);
    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    // Map the buffer's GPU memory to CPU memory, and write vertex info to it.
    const MaterialData materialData{ albedo, emissive, metallic, roughness, alpha };
    void* memMapped;
    vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &memMapped);
    memcpy(memMapped, &materialData, (size_t)bufferSize);
    vkUnmapMemory(vkDevice, stagingBufferMemory);

    // Create the vertex buffer.
    CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 vkDataBuffer, vkDataBufferMemory);

    // Copy the staging buffer to the vertex buffer.
    CopyBuffer(vkDevice, renderer->GetVkCommandPool(), renderer->GetVkGraphicsQueue(), stagingBuffer, vkDataBuffer, bufferSize);

    // De-allocate the staging buffer.
    vkDestroyBuffer(vkDevice, stagingBuffer,       nullptr);
    vkFreeMemory   (vkDevice, stagingBufferMemory, nullptr);
    
    // Allocate the descriptor set.
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = vkDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &vkDescriptorSetLayout;
    if (vkAllocateDescriptorSets(vkDevice, &allocInfo, &vkDescriptorSet) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to allocate descriptor sets.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_SET_ALLOCATION_ERROR");
    }

    // Populate the descriptor set.
    VkDescriptorBufferInfo dataBufferInfo{};
    dataBufferInfo.buffer = vkDataBuffer;
    dataBufferInfo.offset = 0;
    dataBufferInfo.range  = sizeof(MaterialData);
    
    VkDescriptorImageInfo imagesInfo[MaterialTextureType::COUNT];
    for (size_t j = 0; j < MaterialTextureType::COUNT; j++)
    {
        const Texture* texture    = textures[MaterialTextureType::Albedo+j];
        imagesInfo[j].imageView   = texture ? texture->GetVkImageView() : VK_NULL_HANDLE;
        imagesInfo[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imagesInfo[j].sampler     = renderer->GetVkTextureSampler();
    }
    
    VkWriteDescriptorSet descriptorWrites[2] = {{},{}};
    descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet          = vkDescriptorSet;
    descriptorWrites[0].dstBinding      = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo     = &dataBufferInfo;
    descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet          = vkDescriptorSet;
    descriptorWrites[1].dstBinding      = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = MaterialTextureType::COUNT;
    descriptorWrites[1].pImageInfo      = imagesInfo;
    
    vkUpdateDescriptorSets(vkDevice, 2, descriptorWrites, 0, nullptr);
}

void Material::CreateDescriptorLayoutAndPool(const VkDevice& vkDevice)
{
    if (vkDescriptorSetLayout && vkDescriptorPool) return;
     
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
    if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &vkDescriptorSetLayout) != VK_SUCCESS) {
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
    if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &vkDescriptorPool) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create descriptor pool.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_POOL_ERROR");
    }
}

void Material::DestroyDescriptorLayoutAndPool(const VkDevice& vkDevice)
{
    if (vkDescriptorSetLayout) vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, nullptr);
    if (vkDescriptorPool)      vkDestroyDescriptorPool     (vkDevice, vkDescriptorPool,      nullptr);
}
