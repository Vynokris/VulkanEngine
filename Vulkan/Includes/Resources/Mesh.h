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

	public:		
		void LoadObj(const char* filename);

		uint32_t GetVertexCount() const { return (uint32_t)vertices.size(); }
		uint32_t GetIndexCount () const { return (uint32_t)indices .size(); }
		std::vector<Maths::TestVertex> GetVertices() const { return vertices; }
		std::vector<uint32_t>          GetIndices () const { return indices;  }
	};
}
