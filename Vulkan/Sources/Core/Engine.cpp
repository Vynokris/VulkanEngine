#include "Core/Engine.h"
#include "Core/Application.h"
#include "Resources/Camera.h"
#include "Resources/Model.h"
#include "Resources/Mesh.h"
#include "Resources/Texture.h"
using namespace Core;
using namespace Resources;

Engine::Engine()
{
    app = Application::Get();
}

Engine::~Engine()
{
    for (const Model*   model   : models  ) delete model;
    for (const Mesh*    mesh    : meshes  ) delete mesh;
    for (const Texture* texture : textures) delete texture;
    models  .clear();
    meshes  .clear();
    textures.clear();
}

void Engine::Awake()
{
    // Load textures.
    textures.push_back(new Texture("Resources/textures/VikingRoom.png"));

    // Load meshes.
    meshes.push_back(new Mesh("Resources/models/VikingRoom.obj"));
}

void Engine::Start()
{
    // Create camera.
    camera = new Camera({ app->GetWindow()->GetWidth(), app->GetWindow()->GetHeight(), 0, 1, 80 });
    
    // Create models.
    models.push_back(new Model(meshes[0], textures[0]));
}

void Engine::Update(const float& deltaTime) const
{
    // Update model transforms.
    models[0]->transform.Rotate(PI * 0.2f * deltaTime);
}

void Engine::Render(const Renderer* renderer) const
{
    for (const Model* model : models)
        renderer->DrawModel(model, camera);
}
