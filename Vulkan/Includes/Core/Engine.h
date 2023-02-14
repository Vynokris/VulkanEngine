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

	public:
		Engine();
		~Engine();

		void Awake();
		void Start() const;
		void Update(const float& deltaTime) const;
		void Render(const Renderer* renderer) const;
	};
}
