#pragma once
#include "Core/UniqueID.h"
#include "Maths/Transform.h"
#include "Core/GraphicsUtils.h"
#include <vector>

namespace Core { class WavefrontParser; }
namespace Resources
{
	class Camera;
	class Mesh;
	
	class Model : public UniqueID
	{
	private:
		friend Core::WavefrontParser;
		friend Mesh;
		
		std::string       name;
		std::vector<Mesh> meshes;
		
		inline static VkDescriptorSetLayout vkDescriptorSetLayout = nullptr;
		inline static VkDescriptorPool      vkDescriptorPool      = nullptr;
        VkDescriptorSet vkDescriptorSets  [GraphicsUtils::MAX_FRAMES_IN_FLIGHT];
		VkBuffer        vkMvpBuffers      [GraphicsUtils::MAX_FRAMES_IN_FLIGHT];
		VkDeviceMemory  vkMvpBuffersMemory[GraphicsUtils::MAX_FRAMES_IN_FLIGHT];
		void*           vkMvpBuffersMapped[GraphicsUtils::MAX_FRAMES_IN_FLIGHT];

	public:
		Maths::Transform transform;
		
		Model() = default;
		Model(std::string _name, Maths::Transform _transform = Maths::Transform());
		Model(const Model&)            = delete;
		Model(Model&&)                 = delete;
		Model& operator=(const Model&) = delete;
		Model& operator=(Model&&) noexcept;
		~Model();

		void UpdateMvpBuffer(const Camera& camera, const uint32_t& currentFrame) const;
		
		static void CreateVkData (const VkDevice& vkDevice);
		static void DestroyVkData(const VkDevice& vkDevice);
		static VkDescriptorSetLayout GetVkDescriptorSetLayout() { return vkDescriptorSetLayout; }
		static VkDescriptorPool      GetVkDescriptorPool     () { return vkDescriptorPool;      }
               VkDescriptorSet       GetVkDescriptorSet(const uint32_t& currentFrame) const;
		
		std::string              GetName  () const { return name;    }
		const std::vector<Mesh>& GetMeshes() const { return meshes;  }
		      std::vector<Mesh>& GetMeshes()       { return meshes;  }
		
	private:
		void CreateMvpBuffers();
		void CreateDescriptorSets();
	};
}
