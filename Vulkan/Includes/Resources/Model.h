#pragma once
#include "Maths/Transform.h"
#include <vector>

typedef struct VkDescriptorPool_T* VkDescriptorPool;
typedef struct VkDescriptorSet_T*  VkDescriptorSet;
typedef struct VkBuffer_T*         VkBuffer;
typedef struct VkDeviceMemory_T*   VkDeviceMemory;

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
		
		std::string name;
		std::vector<Mesh> meshes;
		
		std::vector<VkBuffer>        vkMvpBuffers;
		std::vector<VkDeviceMemory>  vkMvpBuffersMemory;
		std::vector<void*>           vkMvpBuffersMapped;

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
		
		std::string              GetName  () const { return name;    }
		const std::vector<Mesh>& GetMeshes() const { return meshes;  }

	private:
		void CreateMvpBuffers();
	};
}
