#pragma once
#include <vector>

constexpr unsigned int MAX_LIGHT_COUNT = 20;

namespace Resources
{
	class Camera;
	class Model;
	class Mesh;
	class Texture;
	class Light;
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
		std::vector<Resources::Light*>   lights;
		size_t vertexCount = 0;

	public:
		float cameraSpeed       = 2;
		float cameraSensitivity = 2;
		bool  rotateModels      = false;
		
		Engine();
		~Engine();

		void Awake();
		void Start() const;
		void Update(const float& deltaTime);
		void Render(const Renderer* renderer) const;
		
		void   ResizeCamera(const int& width, const int& height) const;
		void   UpdateVertexCount();
		size_t GetVertexCount() const { return vertexCount; }
	};
}
