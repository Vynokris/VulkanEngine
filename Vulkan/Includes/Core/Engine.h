#pragma once
#include <string>
#include <unordered_map>

namespace Resources
{
	class Camera;
	class Model;
	class Material;
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

		std::vector<Resources::Light>                        lights;
		std::unordered_map<std::string, Resources::Model>    models;
		std::unordered_map<std::string, Resources::Material> materials;
		std::unordered_map<std::string, Resources::Texture>  textures;

	public:
		float cameraSpeed       = 2;
		float cameraSensitivity = 5e-3f;
		bool  rotateModels      = false;
		const std::vector<std::string> defaultResources = {
			// "Resources\\Models\\VikingRoom\\VikingRoom.obj",
			"Resources\\Models\\Stadium\\stadium.obj",
			"Resources\\Models\\DoomSlayer\\doommarine.obj",
		};
		
		Engine();
		Engine(const Engine&)            = delete;
		Engine(Engine&&)                 = delete;
		Engine& operator=(const Engine&) = delete;
		Engine& operator=(Engine&&)      = delete;
		~Engine();

		void Awake();
		void Start() const;
		void Update(const float& deltaTime);
		void Render(const Renderer* renderer) const;

		void LoadFile(const std::string& filename);

		Resources::Model*    GetModel   (const std::string& name);
		Resources::Material* GetMaterial(const std::string& name);
		Resources::Texture*  GetTexture (const std::string& name);
		
		void ResizeCamera(const int& width, const int& height) const;
	};
}
