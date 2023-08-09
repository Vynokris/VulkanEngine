#pragma once
#include "Maths/Vertex.h"
#include "Resources/Material.h"
#include <utility>
#include <vector>

typedef struct VkBuffer_T*       VkBuffer;
typedef struct VkDeviceMemory_T* VkDeviceMemory;

namespace Core { class WavefrontParser; }
namespace Resources
{
	class Model;
	class Texture;
	
	class Mesh
	{
	private:
		friend Core::WavefrontParser;
		
		std::string name;
		Material    material;
		Model&      parentModel;
		
		std::vector<Maths::Vertex> vertices;
		std::vector<uint32_t>      indices;
		
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

		std::string GetName    () const { return name;     }
		Material    GetMaterial() const { return material; }
		void        SetMaterial(const Material& mat) { material = mat; }
		
		uint32_t GetIndexCount    () const { return (uint32_t)indices.size(); }
		VkBuffer GetVkVertexBuffer() const { return vkVertexBuffer; }
		VkBuffer GetVkIndexBuffer () const { return vkIndexBuffer;  }

	private:
		void CreateVertexBuffers();
	};
}
