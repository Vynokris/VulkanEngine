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
		std::unordered_map<std::string, Resources::Model*>   models;
		std::unordered_map<std::string, Resources::Mesh*>    meshes;
		std::unordered_map<std::string, Resources::Texture*> textures;

	public:
		float cameraSpeed       = 2;
		float cameraSensitivity = 5e-3f;
		bool  rotateModels      = false;
		const std::vector<std::string> defaultResources = {
			"Resources\\Textures\\Default.png",
			"Resources\\Meshes\\Quad.obj",
			"Resources\\Meshes\\Cube.obj",
		};
		
		Engine();
		~Engine();

		void Awake();
		void Start() const;
		void Update(const float& deltaTime);
		void Render(const Renderer* renderer) const;

		void LoadFile (const std::string& filename);
		void SaveScene(const std::string& filename) const;
		void QueueSceneLoad(const std::string& filename) { sceneToLoad = filename; }
		
		void ResizeCamera(const int& width, const int& height) const;
		void UpdateVertexCount();
		
		std::string GetSceneName()   const { return sceneName; }
		size_t      GetVertexCount() const { return vertexCount; }

	private:
		void UnloadOutdatedResources();
		
		void LoadScene(const std::string& filename);
		void UnloadScene();
	};
}
