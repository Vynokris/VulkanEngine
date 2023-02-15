#include "Core/Engine.h"
#include "Core/Application.h"
#include "Resources/Camera.h"
#include "Resources/Model.h"
#include "Resources/Mesh.h"
#include "Resources/Texture.h"
using namespace Core;
using namespace Resources;
using namespace Maths;

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
    delete camera;
}

void Engine::Awake()
{
    // Create camera.
    camera = new Camera({ app->GetWindow()->GetWidth(), app->GetWindow()->GetHeight(), 0, 1, 60 });
    
    // Load textures.
    textures.push_back(new Texture("Resources/Textures/VikingRoom.png"));
    textures.push_back(new Texture("Resources/Textures/Banana.png"));
    textures.push_back(new Texture("Resources/Textures/ItemBox.png"));
    textures.push_back(new Texture("Resources/Textures/Rico.png"));

    // Load meshes.
    meshes.push_back(new Mesh("Resources/Meshes/VikingRoom.obj"));
    meshes.push_back(new Mesh("Resources/Meshes/Banana.obj"));
    meshes.push_back(new Mesh("Resources/Meshes/ItemBox.obj"));
    meshes.push_back(new Mesh("Resources/Meshes/Rico.obj"));
    
    // Create models.
    models.push_back(new Model(meshes[0], textures[0]));
    models.push_back(new Model(meshes[1], textures[1]));
    models.push_back(new Model(meshes[2], textures[2]));
    models.push_back(new Model(meshes[3], textures[3]));
}

void Engine::Start() const
{
    // Set camera transform.
    camera->transform.Move({ 0, -1.f, 1.f });
    camera->transform.Rotate(Quaternion::FromPitch(PI * 0.2f));

    // Rotate all models because my matrices were made for OpenGL and not Vulkan.
    // TODO: Fix my matrices.
    for (Model* model : models) {
        model->transform.Rotate(Quaternion::FromEuler({ PI, -PI*0.5f, 0 }));
        model->transform.SetScale({ 0.7f });
    }

    // Set model transforms.
    models[0]->transform.Move({  0,    0, 0 });
    models[1]->transform.Move({ -1.5f, 0, 0 });
    models[2]->transform.Move({ -1.5f, 0, 0 });
    models[3]->transform.Move({  1.5f, 0, 0 });
    models[1]->transform.SetScale({ 0.54f });
}

void Engine::Update(const float& deltaTime) const
{
    // Update model transforms.
    const Quaternion rotQuat = Quaternion::FromRoll(PI * 0.2f * deltaTime);
    for (Model* model : models)
        model->transform.Rotate(rotQuat);
}

void Engine::Render(const Renderer* renderer) const
{
    for (const Model* model : models)
        renderer->DrawModel(model, camera);
}
