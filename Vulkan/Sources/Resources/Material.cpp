#include "Resources/Material.h"
#include "Core/Application.h"
#include <vulkan/vulkan_core.h>

#include "Maths/Vertex.h"
#include "Resources/Texture.h"
using namespace Core;
using namespace Maths;
using namespace Resources;
using namespace VulkanUtils;

Material::Material(const RGB& _albedo, const RGB& _emissive, const float& _shininess, const float& _alpha,
                   Texture* albedoTexture, Texture* emissiveTexture, Texture* shininessMap, Texture* alphaMap, Texture* normalMap)
    : albedo(_albedo), emissive(_emissive), shininess(_shininess), alpha(_alpha)
{
    textures[MaterialTextureType::Albedo   ] = albedoTexture;
    textures[MaterialTextureType::Emissive ] = emissiveTexture;
    textures[MaterialTextureType::Shininess] = shininessMap;
    textures[MaterialTextureType::Alpha    ] = alphaMap;
    textures[MaterialTextureType::Normal   ] = normalMap;
}

Material& Material::operator=(Material&& other) noexcept
{
    name      = other.name;      other.name       = "";
    albedo    = other.albedo;    other.albedo     = 0;
    emissive  = other.emissive;  other.emissive   = 0;
    shininess = other.shininess; other.shininess  = 0;
    alpha     = other.alpha;     other.alpha      = 0;
    for (size_t i = 0; i < textureTypesCount; i++) {
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

void Material::SetParams(const RGB& _albedo, const RGB& _emissive, const float& _shininess, const float& _alpha)
{
    albedo    = _albedo;
    emissive  = _emissive;
    shininess = _shininess;
    alpha     = _alpha;
}

void Material::FinalizeLoading()
{
    if (IsLoadingFinalized()) return;
    
    // Get necessary vulkan variables.
    const Renderer* renderer = Application::Get()->GetRenderer();
    const VkDevice  vkDevice = renderer->GetVkDevice();

    // Create the material data buffer.
    CreateBuffer(vkDevice, renderer->GetVkPhysicalDevice(), sizeof(MaterialData),
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 vkDataBuffer, vkDataBufferMemory);

    // Fill the material data buffer.
    const MaterialData materialData{ albedo, emissive, shininess, alpha };
    void* memMapped;
    vkMapMemory(vkDevice, vkDataBufferMemory, 0, sizeof(MaterialData), 0, &memMapped);
    memcpy(memMapped, &materialData, sizeof(MaterialData));
    vkUnmapMemory(vkDevice, vkDataBufferMemory);
    
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
    
    VkDescriptorImageInfo imagesInfo[textureTypesCount];
    for (size_t j = 0; j < textureTypesCount; j++)
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
    descriptorWrites[1].descriptorCount = textureTypesCount;
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
    layoutBindings[1].descriptorCount = textureTypesCount;
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
    poolSizes[0].descriptorCount = 1000; // TODO: Make this the max number of materials in the scene.
    poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1000; // TODO: Make this the max number of materials in the scene.

    // Create the descriptor pool.
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes    = poolSizes;
    poolInfo.maxSets       = 1000; // TODO: Make this the max number of materials in the scene.
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
