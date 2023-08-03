#pragma once
#include <string>
#include <unordered_map>

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

		std::string sceneName, sceneToLoad;
		size_t      vertexCount = 0;
		std::unordered_map<std::string, Resources::Model  *> models;
		std::unordered_map<std::string, Resources::Texture*> textures;

	public:
		float cameraSpeed       = 2;
		float cameraSensitivity = 5e-3f;
		bool  rotateModels      = false;
		const std::vector<std::string> defaultResources = {
			// "Resources\\Models\\VikingRoom\\VikingRoom.obj",
			// "Resources\\Models\\Stadium\\stadium.obj"
			"Resources\\Models\\DoomSlayer\\doommarine.obj"
		};
		
		Engine();
		Engine(const Engine& other)      = delete;
		Engine(Engine&&)                 = delete;
		Engine& operator=(const Engine&) = delete;
		Engine& operator=(Engine&&)      = delete;
		~Engine();

		void Awake();
		void Start() const;
		void Update(const float& deltaTime);
		void Render(const Renderer* renderer) const;

		void LoadFile(const std::string& filename);

		Resources::Model*   GetModel  (const std::string& name);
		Resources::Texture* GetTexture(const std::string& name);
		
		void ResizeCamera(const int& width, const int& height) const;
	};
}
