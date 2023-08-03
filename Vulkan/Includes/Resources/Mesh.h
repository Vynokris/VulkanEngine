#pragma once
#include "Maths/Vertex.h"
#include "Resources/Material.h"
#include <utility>
#include <vector>

typedef struct VkDescriptorPool_T* VkDescriptorPool;
typedef struct VkDescriptorSet_T*  VkDescriptorSet;
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
		bool        loadingFinalized = false;
		Model&      parentModel;
		
		std::vector<Maths::Vertex> vertices;
		std::vector<uint32_t>      indices;
		
		VkDescriptorPool             vkDescriptorPool = nullptr;
		std::vector<VkDescriptorSet> vkDescriptorSets;
		VkBuffer       vkVertexBuffer       = nullptr;
		VkDeviceMemory vkVertexBufferMemory = nullptr;
		VkBuffer       vkIndexBuffer        = nullptr;
		VkDeviceMemory vkIndexBufferMemory  = nullptr;

	public:
		Mesh(std::string _name, Model& _parentModel) : name(std::move(_name)), parentModel(_parentModel) {}
		Mesh(const Mesh& other)      = delete;
		Mesh(Mesh&&) noexcept;
		Mesh& operator=(const Mesh&) = delete;
		Mesh& operator=(Mesh&&)      = delete;
		~Mesh();

		void FinalizeLoading   ()       { if (loadingFinalized) return; CreateVertexBuffers(); CreateDescriptors(); loadingFinalized = true; }
		bool IsLoadingFinalized() const { return loadingFinalized; }

		std::string GetName    () const { return name;     }
		Material    GetMaterial() const { return material; }
		void        SetMaterial(const Material& mat) { material = mat; }
		
		uint32_t GetIndexCount    () const { return (uint32_t)indices.size(); }
		VkBuffer GetVkVertexBuffer() const { return vkVertexBuffer; }
		VkBuffer GetVkIndexBuffer () const { return vkIndexBuffer;  }
		const VkDescriptorSet& GetVkDescriptorSet(const uint32_t& currentFrame) const { return vkDescriptorSets[currentFrame]; }

	private:
		void CreateVertexBuffers();
		void CreateDescriptors();
	};
}
