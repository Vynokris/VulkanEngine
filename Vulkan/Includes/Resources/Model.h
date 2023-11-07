#pragma once
#include "Maths/Transform.h"
#include <vector>

typedef struct VkDescriptorSetLayout_T* VkDescriptorSetLayout;
typedef struct VkDescriptorPool_T*      VkDescriptorPool;
typedef struct VkBuffer_T*              VkBuffer;
typedef struct VkDeviceMemory_T*        VkDeviceMemory;

namespace Core { class WavefrontParser; }
namespace Resources
{
	class Camera;
	class Mesh;
	
	class Model
	{
	private:
		friend Core::WavefrontParser;
		friend Mesh;
		
		std::string       name;
		std::vector<Mesh> meshes;
		
		inline static VkDescriptorSetLayout vkDescriptorSetLayout = nullptr;
		inline static VkDescriptorPool      vkDescriptorPool      = nullptr;
        VkDescriptorSet vkDescriptorSets  [VkUtils::MAX_FRAMES_IN_FLIGHT];
		VkBuffer        vkMvpBuffers      [VkUtils::MAX_FRAMES_IN_FLIGHT];
		VkDeviceMemory  vkMvpBuffersMemory[VkUtils::MAX_FRAMES_IN_FLIGHT];
		void*           vkMvpBuffersMapped[VkUtils::MAX_FRAMES_IN_FLIGHT];

	public:
		Maths::Transform transform;
		
		Model() = default;
		Model(std::string _name, Maths::Transform _transform = Maths::Transform());
		Model(const Model&)            = delete;
		Model(Model&&)                 = delete;
		Model& operator=(const Model&) = delete;
		Model& operator=(Model&&) noexcept;
		~Model();

		void UpdateMvpBuffer(const Camera* camera, const uint32_t& currentFrame) const;
		
		static void CreateDescriptorLayoutAndPool (const VkDevice& vkDevice);
		static void DestroyDescriptorLayoutAndPool(const VkDevice& vkDevice);
		static VkDescriptorSetLayout GetVkDescriptorSetLayout() { return vkDescriptorSetLayout; }
		static VkDescriptorPool      GetVkDescriptorPool     () { return vkDescriptorPool;      }
               VkDescriptorSet       GetVkDescriptorSet(const uint32_t& currentFrame) const;
		
		std::string              GetName  () const { return name;    }
		const std::vector<Mesh>& GetMeshes() const { return meshes;  }
		
	private:
		void CreateMvpBuffers();
		void CreateDescriptorSets();
	};
}
