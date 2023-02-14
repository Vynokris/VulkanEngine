#pragma once
#include "Maths/Vertex.h"
#include <vector>

typedef struct VkBuffer_T*       VkBuffer;
typedef struct VkDeviceMemory_T* VkDeviceMemory;

namespace Resources
{
	class Mesh
	{
	private:
		std::vector<Maths::TestVertex> vertices;
		std::vector<uint32_t>          indices;
		
		VkBuffer       vkVertexBuffer       = nullptr;
		VkDeviceMemory vkVertexBufferMemory = nullptr;
		VkBuffer       vkIndexBuffer        = nullptr;
		VkDeviceMemory vkIndexBufferMemory  = nullptr;

	public:
		Mesh() = default;
		Mesh(std::vector<Maths::TestVertex> _vertices, std::vector<uint32_t> _indices);
		Mesh(const char* filename);
		~Mesh();

		uint32_t GetVertexCount() const { return (uint32_t)vertices.size(); }
		uint32_t GetIndexCount () const { return (uint32_t)indices .size(); }
		
		VkBuffer GetVkVertexBuffer() const { return vkVertexBuffer; }
		VkBuffer GetVkIndexBuffer()  const { return vkIndexBuffer; }

	private:
		void CreateVertexBuffer();
		void CreateIndexBuffer();
	};
}
