#include "Resources/Light.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "Core/RendererShadows.h"
#include "Core/GpuDataManager.h"
#include "Core/Engine.h"
#include "Core/GraphicsUtils.h"
#include <vulkan/vulkan_core.h>

#include "Maths/AngleAxis.h"
using namespace Resources;
using namespace Maths;
using namespace Core;

Light Light::Directional(const Vector3& _direction, const RGBA& _albedo)
{
    Light newLight{};
    newLight.type      = LightType::Directional;
    newLight.albedo    = _albedo;
    newLight.direction = _direction;
    return newLight;
}

Light Light::Point(const Vector3& _position, const RGBA& _albedo,
                   const float& _radius, const float& _falloff)
{
    Light newLight{};
    newLight.type     = LightType::Point;
    newLight.albedo   = _albedo;
    newLight.position = _position;
    newLight.radius   = _radius;
    newLight.falloff  = _falloff;
    return newLight;
}

Light Light::Spot(const Vector3& _position, const Vector3& _direction, const RGBA& _albedo,
                  const float& _radius, const float& _falloff,
                  const float& _outerCutoff, const float& _innerCutoff)
{
    return { _albedo, _position, _direction, _radius, _falloff, _outerCutoff, _innerCutoff, LightType::Spot };
}

void Light::UpdateBufferData(const std::vector<Light>& lights, const GpuArray<Light>* lightsArray)
{
    const Application*     app             = Application::Get();
          Engine*          engine          = app->GetEngine();
    const RendererShadows* rendererShadows = app->GetRenderer()->GetRendererShadows();
    if (!lightsArray) lightsArray = &app->GetGpuData()->GetArray<Light>();

    const Light& light = *engine->GetLight(0);
    Mat4 lightViewProj[4] = {{false},{false},{false},{false}};
    switch (light.type)
    {
    case LightType::Directional:
        {
            // Compute the directional light's orthogonal projection matrix.
            constexpr float halfWidth  = 2;
            constexpr float halfHeight = 2;
            constexpr float r = halfWidth, l = -halfWidth, t = halfHeight, b = -halfHeight, n = .1f, f = 15.f;
            const Mat4 lightProj = Mat4(
                2.f/(r-l), 0.f, 0.f, 0.f,
                0.f, 2.f/(b-t), 0.f, 0.f,
                0.f, 0.f, -2.f/(f-n), 0.f,
                -(r+l)/(r-l), -(b+t)/(b-t), 1.f-(f+n)/(f-n), 1.f
            );
        
            // Compute the directional light's view matrix using its direction.
            const Vector3 lightPos = -light.direction;
            const Transform lightTransform = {
                lightPos,
                Quaternion::LookAt(lightPos, Vector3::Zero()),
                { 1, 1, 1 },
                true
            };
            const Mat4 lightView = lightTransform.GetViewMat();

            lightViewProj[0] = lightView * lightProj;
        }
        break;
        
    case LightType::Point:
        {
            // Create the shared perspective projection for all 4 faces of the tetrahedron.
            constexpr float fovX = 143.985709f * DEGTORAD;
            constexpr float fovY = 125.264389f * DEGTORAD;
            const float aspect = tan(fovX * 0.5f) / tan(fovY * 0.5f);
            const float yScale = 1 / tan(fovY * 0.5f);
            const float xScale = yScale / aspect;
            constexpr float n = 0.1f, f = 50.f;
            const Mat4 lightProj = Mat4(
                xScale, 0, 0, 0,
                0, -yScale, 0, 0,
                0, 0, f/(n-f), -1,
                0, 0, f*n/(n-f), 0
            );
            
            // Define orientations for each face of the tetrahedron.
            constexpr float pitch = 35.26438916f * DEGTORAD;
            const Vector3 position = light.position;
            const Quaternion faceRotations[4] = {
                Quaternion::LookTowards(Vector3(0.0f, -0.57735026f, 0.81649661f)).GetConjugate(), // Green  Face
                Quaternion::FromYaw(      PI) * Quaternion::FromPitch( pitch),                    // Yellow Face
                Quaternion::FromRoll(-PIDIV2) * Quaternion::FromPitch(-pitch),                    // Blue   Face
                Quaternion::FromRoll( PIDIV2) * Quaternion::FromPitch(-pitch),                    // Red    Face
            };

            // Compute the view matrix of all faces.
            for (size_t i = 0; i < 4; i++)
                lightViewProj[i] = Mat4::FromTranslation(-position) * Mat4::FromQuaternion(faceRotations[i]) * lightProj;
        }
        break;
        
    case LightType::Spot:
        {
            // Compute the spot light's perspective projection matrix.
            const float aspect = (float)rendererShadows->GetVkShadowImageWidth() / (float)rendererShadows->GetVkShadowImageHeight();
            const float fov    = remap(-1, 1, 0, PI, light.outerCutoff);
            const float yScale = 1 / tan(fov * 0.5f);
            const float xScale = yScale / aspect;
            constexpr float n = 0.1f, f = 50.f;
            const Mat4 lightProj = Mat4(
                xScale, 0, 0, 0,
                0, -yScale, 0, 0,
                0, 0, f/(n-f), -1,
                0, 0, f*n/(n-f), 0
            );
        
            // Compute the spot light's view matrix using its position and direction.
            const Transform lightTransform = {
                light.position,
                Quaternion::LookTowards(light.direction),
                { 1, 1, 1 },
                true
            };
            const Mat4 lightView = lightTransform.GetViewMat();

            lightViewProj[0] = lightView * lightProj;
        }
        break;
        
    case LightType::Unassigned:
    default:
        break;
    }
    
    // Update buffer data.
    memcpy(rendererShadows->GetVkBufferMapped(), lightViewProj, rendererShadows->GetVkBufferSize());
    memcpy(lightsArray->vkBufferMapped, lights.data(), sizeof(Light) * Engine::MAX_LIGHTS);
}


template<> const GpuArray<Light>& GpuDataManager::CreateArray()
{
    using namespace GraphicsUtils;
    if (lightsArray.vkDescriptorSetLayout && lightsArray.vkDescriptorPool && lightsArray.vkBuffer && lightsArray.vkBufferMemory) return lightsArray;

    // Get the necessary vulkan resources.
    const VkDevice         vkDevice         = renderer->GetVkDevice();
    const VkPhysicalDevice vkPhysicalDevice = renderer->GetVkPhysicalDevice();

    // Set the binding of the data buffer object.
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding         = 0;
    layoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Enable modification of the light data after it is bound to a command buffer.
    constexpr VkDescriptorBindingFlags bindingFlag = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    VkDescriptorSetLayoutBindingFlagsCreateInfo extendedLayoutInfo{};
    extendedLayoutInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    extendedLayoutInfo.bindingCount  = 1;
    extendedLayoutInfo.pBindingFlags = &bindingFlag;
    
    // Create the descriptor set layout.
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings    = &layoutBinding;
    layoutInfo.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    layoutInfo.pNext        = &extendedLayoutInfo;
    if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &lightsArray.vkDescriptorSetLayout) != VK_SUCCESS) {
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
    poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &lightsArray.vkDescriptorPool) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create descriptor pool.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_POOL_ERROR");
    }

    // Create the light data buffer.
    lightsArray.vkBufferSize = sizeof(Light) * Engine::MAX_LIGHTS;
    CreateBuffer(vkDevice, vkPhysicalDevice, lightsArray.vkBufferSize,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                 lightsArray.vkBuffer, lightsArray.vkBufferMemory);
    vkMapMemory(vkDevice, lightsArray.vkBufferMemory, 0, lightsArray.vkBufferSize, 0, &lightsArray.vkBufferMapped);

    // Allocate the descriptor set.
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = lightsArray.vkDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &lightsArray.vkDescriptorSetLayout;
    if (vkAllocateDescriptorSets(vkDevice, &allocInfo, &lightsArray.vkDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    // Define info for the light data buffer.
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = lightsArray.vkBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range  = lightsArray.vkBufferSize;

    // Populate the descriptor set.
    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet          = lightsArray.vkDescriptorSet;
    descriptorWrite.dstBinding      = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo     = &bufferInfo;
    vkUpdateDescriptorSets(vkDevice, 1, &descriptorWrite, 0, nullptr);

    return lightsArray;
}