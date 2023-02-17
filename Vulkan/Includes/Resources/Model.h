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
		std::string name;
		Mesh*       mesh    = nullptr;
		Texture*    texture = nullptr;
		
		VkDescriptorPool             vkDescriptorPool = nullptr;
		std::vector<VkDescriptorSet> vkDescriptorSets;
		std::vector<VkBuffer>        vkMvpBuffers;
		std::vector<VkDeviceMemory>  vkMvpBuffersMemory;
		std::vector<void*>           vkMvpBuffersMapped;

	public:
		Maths::Transform transform;
		bool shouldDelete = false;

		Model(std::string _name, Mesh* _mesh, Texture* _texture, Maths::Transform _transform = Maths::Transform());
		~Model();

		void UpdateMvpBuffer(const Camera* camera, const uint32_t& currentFrame) const;

		std::string GetName   () const { return name;    }
		Mesh*       GetMesh   () const { return mesh;    }
		Texture*    GetTexture() const { return texture; }

		const VkDescriptorSet& GetDescriptorSet(const uint32_t& currentFrame) const { return vkDescriptorSets[currentFrame]; }

	private:
		void CreateMvpBuffers();
		void CreateDescriptorPool();
		void CreateDescriptorSets();
	};
}
