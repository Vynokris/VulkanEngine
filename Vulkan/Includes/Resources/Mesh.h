#pragma once
#include "Core/UniqueID.h"
#include "Maths/Vertex.h"
#include "Resources/Material.h"
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

typedef struct VkBuffer_T*       VkBuffer;
typedef struct VkDeviceMemory_T* VkDeviceMemory;

namespace Core { class WavefrontParser; }
namespace Resources
{
	class Model;
	class Texture;
	
	class Mesh : public UniqueID
	{
	private:
		friend Core::WavefrontParser;
		
		std::string name;
		Material*   material = nullptr;
		Model&      parentModel;
		
		std::vector<Maths::TangentVertex> vertices;
		std::vector<uint32_t>             indices;
		
		VkBuffer       vkVertexBuffer       = nullptr;
		VkDeviceMemory vkVertexBufferMemory = nullptr;
		VkBuffer       vkIndexBuffer        = nullptr;
		VkDeviceMemory vkIndexBufferMemory  = nullptr;

	public:
		Mesh(std::string _name, Model& _parentModel) : name(std::move(_name)), parentModel(_parentModel) {}
		Mesh(const Mesh&)            = delete;
		Mesh(Mesh&&) noexcept;
		Mesh& operator=(const Mesh&) = delete;
		Mesh& operator=(Mesh&&)      = delete;
		~Mesh();

		bool IsLoadingFinalized() const { return vkVertexBuffer && vkVertexBufferMemory && vkIndexBuffer && vkIndexBufferMemory; }
		void FinalizeLoading   ()       { if (IsLoadingFinalized()) return; CreateVertexBuffers(); }

		std::string     GetName    () const { return name;     }
		const Material* GetMaterial() const { return material; }
		void            SetMaterial(Material* _material) { material = _material; }

		const std::vector<Maths::TangentVertex>& GetVertices() const { return vertices; }
		const std::vector<uint32_t>&             GetIndices()  const { return indices; }
		uint32_t GetIndexCount    () const { return (uint32_t)indices.size(); }
		VkBuffer GetVkVertexBuffer() const { return vkVertexBuffer; }
		VkBuffer GetVkIndexBuffer () const { return vkIndexBuffer;  }
		
		static VkVertexInputBindingDescription GetVertexBindingDescription();
		static std::array<VkVertexInputAttributeDescription, 5> GetVertexAttributeDescriptions();

	private:
		void CreateVertexBuffers();
	};
}
