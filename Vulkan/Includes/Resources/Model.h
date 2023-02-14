#pragma once
#include "Maths/Transform.h"
#include <vector>

typedef struct VkDescriptorSet_T* VkDescriptorSet;
typedef struct VkBuffer_T*        VkBuffer;
typedef struct VkDeviceMemory_T*  VkDeviceMemory;

namespace Resources
{
	class Mesh;
	class Texture;
	
	class Model
	{
	private:
		Mesh*    mesh    = nullptr;
		Texture* texture = nullptr;
		
		std::vector<VkDescriptorSet> vkDescriptorSets;
		std::vector<VkBuffer>        vkUniformBuffers;
		std::vector<VkDeviceMemory>  vkUniformBuffersMemory;
		std::vector<void*>           vkUniformBuffersMapped;

	public:
		Maths::Transform transform;

		Model(Mesh* _mesh, Texture* _texture, Maths::Transform _transform = Maths::Transform())
			: mesh(_mesh), texture(_texture), transform(std::move(_transform)) {}

		Mesh*    GetMesh   () const { return mesh;    }
		Texture* GetTexture() const { return texture; }

	private:
		void CreateDescriptorSets();
		void CreateUniformBuffers();
	};
}
