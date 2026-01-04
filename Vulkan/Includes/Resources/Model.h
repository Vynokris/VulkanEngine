#pragma once
#include "Core/UniqueID.h"
#include "Maths/Transform.h"
#include "Core/GraphicsUtils.h"
#include <vector>
#include <optional>

namespace Core { class WavefrontParser; template<typename T> struct GpuData; }
namespace Resources
{
	class Camera;
	class Mesh;
	
	class Model : public UniqueID
	{
	private:
		friend Core::WavefrontParser;
		friend Mesh;
		
		std::string       name;
		std::vector<Mesh> meshes;

	public:
		std::vector<Maths::Transform> transforms;
		
		Model() = default;
		Model(std::string _name);
		Model(const Model&)            = delete;
		Model(Model&&)                 = delete;
		Model& operator=(const Model&) = delete;
		Model& operator=(Model&&) noexcept;
		~Model();

		void UpdateMvpBuffer(const uint32_t& currentFrame, const Core::GpuData<Model>* modelData = nullptr) const;
		
		std::string              GetName  () const { return name;    }
		const std::vector<Mesh>& GetMeshes() const { return meshes;  }
		      std::vector<Mesh>& GetMeshes()       { return meshes;  }
	};
}
