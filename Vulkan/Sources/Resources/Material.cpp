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
    memset(vkDescriptorSets, NULL, sizeof(vkDescriptorSets));
}

void Material::SetParams(const RGB& _albedo, const RGB& _emissive, const float& _shininess, const float& _alpha)
{
    albedo    = _albedo;
    emissive  = _emissive;
    shininess = _shininess;
    alpha     = _alpha;
}

bool Material::IsLoadingFinalized() const
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (!vkDescriptorSets[i]) {
            return false;
        }
    }
    return true;
}

void Material::FinalizeLoading()
{
    if (IsLoadingFinalized()) return;
    
    // Get necessary vulkan variables.
    const Renderer* renderer = Application::Get()->GetRenderer();
    const VkDevice  vkDevice = renderer->GetVkDevice();
    
    // Allocate the descriptor sets.
    const std::vector layouts(MAX_FRAMES_IN_FLIGHT, vkDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = vkDescriptorPool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts        = layouts.data();
    if (vkAllocateDescriptorSets(vkDevice, &allocInfo, vkDescriptorSets) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to allocate descriptor sets.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_SET_ALLOCATION_ERROR");
    }

    // Populate the descriptor sets.
    VkDescriptorImageInfo imagesInfo[textureTypesCount];
    for (size_t j = 0; j < textureTypesCount; j++)
    {
        const Texture* texture    = textures[MaterialTextureType::Albedo+j];
        imagesInfo[j].imageView   = texture ? texture->GetVkImageView() : VK_NULL_HANDLE;
        imagesInfo[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imagesInfo[j].sampler     = renderer->GetVkTextureSampler();
    }
    for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet          = vkDescriptorSets[i];
        descriptorWrite.dstBinding      = 1;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = textureTypesCount;
        descriptorWrite.pImageInfo      = imagesInfo;
        
        vkUpdateDescriptorSets(vkDevice, 1, &descriptorWrite, 0, nullptr);
    }
}

void Material::CreateDescriptorLayoutAndPool(const VkDevice& vkDevice)
{
     if (vkDescriptorSetLayout && vkDescriptorPool) return;
     
    // Set the binding of the mvp buffer object.
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding         = 1;
    samplerLayoutBinding.descriptorCount = textureTypesCount;
    samplerLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Create the descriptor set layout.
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings    = &samplerLayoutBinding;
    if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &vkDescriptorSetLayout) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create descriptor set layout.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_SET_LAYOUT_ERROR");
    }

    // Set the type and number of descriptors.
    VkDescriptorPoolSize poolSize{};
    poolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT * 1000; // TODO: Multiply by the max number of materials in the scene.

    // Create the descriptor pool.
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = &poolSize;
    poolInfo.maxSets       = MAX_FRAMES_IN_FLIGHT * 1000;
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

VkDescriptorSet Material::GetVkDescriptorSet(const uint32_t& currentFrame) const
{
    if (currentFrame > MAX_FRAMES_IN_FLIGHT)
        return nullptr;
    return vkDescriptorSets[currentFrame];
}
