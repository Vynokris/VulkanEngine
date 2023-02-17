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
		const char* filename;
		std::vector<Maths::Vertex> vertices;
		std::vector<uint32_t>      indices;
		
		VkBuffer       vkVertexBuffer       = nullptr;
		VkDeviceMemory vkVertexBufferMemory = nullptr;
		VkBuffer       vkIndexBuffer        = nullptr;
		VkDeviceMemory vkIndexBufferMemory  = nullptr;

	public:
		Mesh() = default;
		Mesh(const char* _filename, std::vector<Maths::Vertex> _vertices, std::vector<uint32_t> _indices);
		Mesh(const char* _filename);
		~Mesh();

		const char* GetFilename   () const { return filename; }
		uint32_t GetIndexCount    () const { return (uint32_t)indices.size(); }
		VkBuffer GetVkVertexBuffer() const { return vkVertexBuffer; }
		VkBuffer GetVkIndexBuffer () const { return vkIndexBuffer;  }

	private:
		void CreateVertexBuffer();
		void CreateIndexBuffer();
	};
}
