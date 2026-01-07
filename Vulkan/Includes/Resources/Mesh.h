#pragma once
#include "Core/UniqueID.h"
#include "Maths/Vertex.h"
#include "Resources/Material.h"
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Core { class WavefrontParser; }
namespace Resources
{
	class Model;
	class Texture;
	
	class Mesh : public UniqueID
	{
	public:
		struct Section
		{
			uint32_t vtxOffset;
			uint32_t idxOffset;
			uint32_t vtxCount;
			uint32_t idxCount;
		};

	private:
		friend Core::WavefrontParser;
		
		std::string name;
		Material*   material = nullptr;
		Model&      parentModel;
		
		std::vector<Maths::TangentVertex> vertices;
		std::vector<uint32_t>             indices;
		std::vector<Section>              sections;

	public:
		Mesh(std::string _name, Model& _parentModel) : name(std::move(_name)), parentModel(_parentModel) {}
		Mesh(const Mesh&)            = delete;
		Mesh(Mesh&&) noexcept;
		Mesh& operator=(const Mesh&) = delete;
		Mesh& operator=(Mesh&&)      = delete;
		~Mesh();

		std::string     GetName    () const { return name;     }
		const Material* GetMaterial() const { return material; }
		void            SetMaterial(Material* _material) { material = _material; }

		const std::vector<Maths::TangentVertex>& GetVertices() const { return vertices; }
		const std::vector<uint32_t>&             GetIndices()  const { return indices; }
		const std::vector<Section>&              GetSections() const { return sections; }

		uint32_t GetVertexCount()  const { return (uint32_t)vertices.size(); }
		uint32_t GetIndexCount()   const { return (uint32_t)indices.size(); }
		uint32_t GetSectionCount() const { return (uint32_t)sections.size(); }
		bool     HasSections()     const { return GetSectionCount() > 1; }
		
		static VkVertexInputBindingDescription GetVertexBindingDescription();
		static std::array<VkVertexInputAttributeDescription, 5> GetVertexAttributeDescriptions();

	private:
		void StartNewSection();
		void FinalizeLoading();
	};
}
