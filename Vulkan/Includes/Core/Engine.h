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
	public:
		static constexpr unsigned int MAX_LIGHTS = 5;
		static constexpr unsigned int MAX_MODELS = 10;
		static constexpr unsigned int MAX_MATERIALS = 50;
		
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
		const std::vector<std::string> defaultResources = {
			R"(Resources\Models\Stadium\stadium.obj)",
			R"(Resources\Meshes\Sphere.obj)",
			R"(Resources\Meshes\Cube.obj)",
			R"(Resources\Materials\GothicSculptedWall\GothicSculptedWall.mtl)",
			R"(Resources\Models\Headcrab\headcrab.obj)",
			R"(Resources\Models\Gizmo\gizmoTranslation.obj)",
			// R"(Resources\Models\DoomSlayer\doommarine.obj)",
		};
		
		Engine();
		Engine(const Engine&)            = delete;
		Engine(Engine&&)                 = delete;
		Engine& operator=(const Engine&) = delete;
		Engine& operator=(Engine&&)      = delete;
		~Engine();

		void Awake();
		void Start();
		void Update(const float& deltaTime);
		void Render(const Renderer* renderer) const;

		void LoadFile(const std::string& filename);

		Resources::Camera*   GetCamera() const { return camera; }
		Resources::Model*    GetModel   (const std::string& name);
		Resources::Material* GetMaterial(const std::string& name);
		Resources::Texture*  GetTexture (const std::string& name);
		
		void ResizeCamera(const int& width, const int& height) const;
	};
}
