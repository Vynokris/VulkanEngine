#include "Core/Application.h"
#include "Resources/Model.h"
#include "Resources/Camera.h"
#include "Resources/Mesh.h"
#include "Maths/Vertex.h"
#include <vulkan/vulkan.h>
using namespace Core;
using namespace VulkanUtils;
using namespace Resources;
using namespace Maths;

Model::Model(std::string _name, Transform _transform)
     : name(std::move(_name)), transform(std::move(_transform))
{
     transform.SetRotation({ 0, 1, 0, 0 });
     CreateMvpBuffers();
}

Model& Model::operator=(Model&& other) noexcept
{
     name               = other.name;
     meshes             = std::move(other.meshes);
     vkMvpBuffers       = std::move(other.vkMvpBuffers);
     vkMvpBuffersMemory = std::move(other.vkMvpBuffersMemory);
     vkMvpBuffersMapped = std::move(other.vkMvpBuffersMapped);
     transform          = std::move(other.transform);
     other.name = "";
     other.meshes            .clear();
     other.vkMvpBuffers      .clear();
     other.vkMvpBuffersMemory.clear();
     other.vkMvpBuffersMapped.clear();
     other.transform = {};
     return *this;
}

Model::~Model()
{
     const VkDevice vkDevice = Application::Get()->GetRenderer()->GetVkDevice();

     if (vkMvpBuffers.empty() && vkMvpBuffersMemory.empty()) return;
     for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
          if (vkMvpBuffers[i])       vkDestroyBuffer(vkDevice, vkMvpBuffers[i],       nullptr);
          if (vkMvpBuffersMemory[i]) vkFreeMemory   (vkDevice, vkMvpBuffersMemory[i], nullptr);
     }
}

void Model::UpdateMvpBuffer(const Camera* camera, const uint32_t& currentFrame) const
{
     // Copy the matrices to buffer memory.
     const MvpBuffer mvp = { transform.GetLocalMat(), camera->GetViewMat(), camera->GetProjMat() };
     memcpy(vkMvpBuffersMapped[currentFrame], &mvp, sizeof(mvp));
}

void Model::CreateMvpBuffers()
{
     // Get necessary vulkan variables.
     const Renderer*        renderer         = Application::Get()->GetRenderer();
     const VkDevice         vkDevice         = renderer->GetVkDevice();
     const VkPhysicalDevice vkPhysicalDevice = renderer->GetVkPhysicalDevice();

     // Find the size of the buffers and resize the buffer arrays.
     const VkDeviceSize bufferSize = sizeof(MvpBuffer);
     vkMvpBuffers      .resize(MAX_FRAMES_IN_FLIGHT);
     vkMvpBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
     vkMvpBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

     // Create the buffers.
     for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
     {
          CreateBuffer(vkDevice, vkPhysicalDevice, bufferSize,
                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       vkMvpBuffers[i], vkMvpBuffersMemory[i]);

          vkMapMemory(vkDevice, vkMvpBuffersMemory[i], 0, bufferSize, 0, &vkMvpBuffersMapped[i]);
     }
}
