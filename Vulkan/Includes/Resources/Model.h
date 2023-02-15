#pragma once
#include "Maths/Transform.h"
#include <vector>

typedef struct VkDescriptorPool_T* VkDescriptorPool;
typedef struct VkDescriptorSet_T*  VkDescriptorSet;
typedef struct VkBuffer_T*         VkBuffer;
typedef struct VkDeviceMemory_T*   VkDeviceMemory;

namespace Resources
{
	class Camera;
	class Mesh;
	class Texture;
	
	class Model
	{
	private:
		const char* name;
		Mesh*       mesh    = nullptr;
		Texture*    texture = nullptr;
		
		VkDescriptorPool             vkDescriptorPool = nullptr;
		std::vector<VkDescriptorSet> vkDescriptorSets;
		std::vector<VkBuffer>        vkUniformBuffers;
		std::vector<VkDeviceMemory>  vkUniformBuffersMemory;
		std::vector<void*>           vkUniformBuffersMapped;

	public:
		Maths::Transform transform;
		bool shouldDelete = false;

		Model(const char* _name, Mesh* _mesh, Texture* _texture, Maths::Transform _transform = Maths::Transform());
		~Model();

		void UpdateUniformBuffer(const Camera* camera, const uint32_t& currentFrame) const;

		const char* GetName   () const { return name;    }
		Mesh*       GetMesh   () const { return mesh;    }
		Texture*    GetTexture() const { return texture; }

		const VkDescriptorSet& GetDescriptorSet(const uint32_t& currentFrame) const { return vkDescriptorSets[currentFrame]; }

	private:
		void CreateUniformBuffers();
		void CreateDescriptorPool();
		void CreateDescriptorSets();
	};
}
