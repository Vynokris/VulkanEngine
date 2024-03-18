#include "Resources/Light.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "Core/GpuDataManager.h"
#include "Core/Engine.h"
#include "Core/GraphicsUtils.h"
#include <vulkan/vulkan_core.h>

using namespace Resources;
using namespace Maths;
using namespace Core;

Light Light::Directional(const RGB& _albedo, const Vector3& _direction, const float& _brightness)
{
    return { LightType::Directional, _albedo, {}, _direction, _brightness };
}

Light Light::Point(const RGB& _albedo, const Vector3& _position, const float& _brightness,
                   const float& _radius, const float& _falloff)
{
    return { LightType::Point, _albedo, _position, {}, _brightness, _radius, _falloff };
}

Light Light::Spot(const RGB& _albedo, const Vector3& _position, const Vector3& _direction, const float& _brightness,
                  const float& _radius, const float& _falloff,
                  const float& _outerCutoff, const float& _innerCutoff)
{
    return { LightType::Spot, _albedo, _position, _direction, _brightness, _radius, _falloff, _outerCutoff, _innerCutoff };
}

void Light::UpdateBufferData(const std::vector<Light>& lights, const GpuArray<Light>* lightArray)
{
    const Application* app = Application::Get();
    const VkDevice vkDevice = app->GetRenderer()->GetVkDevice();
    if (!lightArray) lightArray = &app->GetGpuData()->GetArray<Light>();

    // Update buffer data.
    void* memMapped;
    vkMapMemory(vkDevice, lightArray->vkBufferMemory, 0, sizeof(Light) * Engine::MAX_LIGHTS, 0, &memMapped);
    memcpy(memMapped, lights.data(), sizeof(Light) * Engine::MAX_LIGHTS);
    vkUnmapMemory(vkDevice, lightArray->vkBufferMemory);
    
    // Update the descriptor set with new data.
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = lightArray->vkBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range  = sizeof(Light) * Engine::MAX_LIGHTS;

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet          = lightArray->vkDescriptorSet;
    descriptorWrite.dstBinding      = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo     = &bufferInfo;

    vkUpdateDescriptorSets(vkDevice, 1, &descriptorWrite, 0, nullptr);
}


template<> const GpuArray<Light>& GpuDataManager::CreateArray()
{
    using namespace GraphicsUtils;
    if (lightArray.vkDescriptorSetLayout && lightArray.vkDescriptorPool && lightArray.vkBuffer && lightArray.vkBufferMemory) return lightArray;

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
    if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &lightArray.vkDescriptorSetLayout) != VK_SUCCESS) {
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
    if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &lightArray.vkDescriptorPool) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create descriptor pool.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_POOL_ERROR");
    }

    // Create the light data buffer.
    CreateBuffer(vkDevice, vkPhysicalDevice, sizeof(Light) * Engine::MAX_LIGHTS,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                 lightArray.vkBuffer, lightArray.vkBufferMemory);

    // Allocate the descriptor set.
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = lightArray.vkDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &lightArray.vkDescriptorSetLayout;
    if (vkAllocateDescriptorSets(vkDevice, &allocInfo, &lightArray.vkDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    return lightArray;
}
