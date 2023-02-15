#pragma once
#include <vector>

namespace Resources
{
	class Camera;
	class Model;
	class Mesh;
	class Texture;
}

namespace Core
{
    class Application;
	class Renderer;
	
	class Engine
	{
	private:
        Application*       app    = nullptr;
        Resources::Camera* camera = nullptr;
		
		std::vector<Resources::Model*>   models;
		std::vector<Resources::Mesh*>    meshes;
		std::vector<Resources::Texture*> textures;
		size_t vertexCount = 0;

	public:
		bool rotateModels = false;
		
		Engine();
		~Engine();

		void Awake();
		void Start() const;
		void Update(const float& deltaTime);
		void Render(const Renderer* renderer) const;

		void   UpdateVertexCount();
		size_t GetVertexCount() const { return vertexCount; }
	};
}
