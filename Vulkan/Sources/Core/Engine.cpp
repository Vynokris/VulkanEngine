#include "Core/Engine.h"
#include "Core/Application.h"
#include "Resources/Camera.h"
#include "Resources/Model.h"
#include "Resources/Mesh.h"
#include "Resources/Texture.h"
#include <filesystem>
namespace fs = std::filesystem;
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
    models.push_back(new Model("VikingRoom", meshes[0], textures[0]));
    models.push_back(new Model("Banana",     meshes[1], textures[1]));
    models.push_back(new Model("ItemBox",    meshes[2], textures[2]));
    models.push_back(new Model("Rico",       meshes[3], textures[3]));

    UpdateVertexCount();
    app->GetUi()->SetResourceRefs(camera, &models, &meshes, &textures);
}

void Engine::Start() const
{
    // Set camera transform.
    camera->transform.Move({ 0, -1.f, 1.f });
    camera->transform.Rotate(Quaternion::FromPitch(PI * 0.2f));

    // Rotate all models because my matrices were made for OpenGL and not Vulkan.
    // TODO: Fix my matrices.
    for (Model* model : models) {
        model->transform.Rotate(Quaternion::FromEuler({ PI, 0, 0 }));
        model->transform.SetScale({ 0.7f });
    }

    // Set model transforms.
    models[1]->transform.Move({ -1.5f, 0, 0 });
    models[2]->transform.Move({ -1.5f, 0, 0 });
    models[3]->transform.Move({  1.5f, 0, 0 });
    models[1]->transform.SetScale({ 0.54f });
}

void Engine::Update(const float& deltaTime)
{
    // Remove out of date models.
    bool wasModelRemoved = false;
    for (int i = (int)models.size()-1; i >= 0; i--) {
        if (models[i]->shouldDelete) {
            delete models[i];
            models.erase(models.begin()+i);
            wasModelRemoved = true;
        }
    }
    if (wasModelRemoved)
        UpdateVertexCount();
    
    // Update model transforms.
    if (rotateModels)
    {
        const Quaternion rotQuat = Quaternion::FromRoll(PI * 0.2f * deltaTime);
        for (Model* model : models)
            model->transform.Rotate(rotQuat);
    }

    // Update camera transform.
    const WindowInputs inputs = app->GetWindow()->GetInputs();
    if (inputs.mouseRightClick)
    {
        camera->transform.Move(camera->transform.GetRotation().RotateVec(inputs.dirMovement * cameraSpeed * deltaTime));
        camera->transform.Rotate(Quaternion::FromAngleAxis({  inputs.mouseDelta.y * cameraSensitivity * deltaTime, camera->transform.Right() }));
        camera->transform.Rotate(Quaternion::FromRoll     (  -inputs.mouseDelta.x * cameraSensitivity * deltaTime ));
    }
}

void Engine::Render(const Renderer* renderer) const
{
    for (const Model* model : models)
        renderer->DrawModel(model, camera);
}

void Engine::LoadFile(const std::string& filename)
{
    const fs::path    path = fs::relative(filename);
    const std::string extension = path.extension().string();
    if (extension == ".obj")
    {
        meshes.push_back(new Mesh(path.string()));
    }
    if (extension == ".jpg" ||
        extension == ".png")
    {
        textures.push_back(new Texture(path.string()));
    }
}

void Engine::ResizeCamera(const int& width, const int& height) const
{
    const CameraParams params = camera->GetParams();
    camera->ChangeParams({ width, height, params.near, params.far, params.fov });
}

void Engine::UpdateVertexCount()
{
    vertexCount = 0;
    for (const Model* model : models)
        vertexCount += model->GetMesh()->GetIndexCount();
}
