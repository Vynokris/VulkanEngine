#include "Core/Engine.h"
#include "Core/Application.h"
#include "Maths/MathConstants.h"
#include "Maths/AngleAxis.h"
#include "Resources/Camera.h"
#include "Resources/Model.h"
#include "Resources/Mesh.h"
#include "Resources/Texture.h"
#include <filesystem>
#include <fstream>

#include "Core/WavefrontParser.h"
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
    delete camera;
}

void Engine::Awake()
{
    // Create camera.
    camera = new Camera({ app->GetWindow()->GetWidth(), app->GetWindow()->GetHeight(), 0.1f, 100, 80 });
    
    // Update the vertex count and set the UI's resource pointers.
    app->GetUi()->SetResourceRefs(camera, &models, &textures);

    // Load default resources.
    for (const std::string& filename : defaultResources)
        LoadFile(filename);
}

void Engine::Start() const
{
    // Set camera transform.
    camera->transform.Move({ 0, -1.f, 1.f });
    camera->transform.Rotate(Quaternion::FromPitch(PI * 0.2f));
}

void Engine::Update(const float& deltaTime)
{
    // Update model transforms.
    if (rotateModels)
    {
        const Quaternion rotQuat = Quaternion::FromRoll(PI * 0.2f * deltaTime);
        for (auto& [name, model] : models)
            model.transform.Rotate(rotQuat);
    }

    // Update camera transform.
    const WindowInputs inputs = app->GetWindow()->GetInputs();
    if (inputs.mouseRightClick)
    {
        camera->transform.Move(camera->transform.GetRotation().RotateVec(inputs.dirMovement * cameraSpeed * deltaTime));
        camera->transform.Rotate(Quaternion::FromAngleAxis({  inputs.mouseDelta.y * cameraSensitivity, camera->transform.Right() }));
        camera->transform.Rotate(Quaternion::FromRoll     (  -inputs.mouseDelta.x * cameraSensitivity ));
    }
}

void Engine::Render(const Renderer* renderer) const
{
    for (const auto& [name, model] : models)
        renderer->DrawModel(model, camera);
}

void Engine::LoadFile(const std::string& filename)
{
    const fs::path    path = fs::relative(filename);
    const std::string extension = path.extension().string();
    if (extension == ".obj")
    {
        std::unordered_map<std::string, Model> newModels = WavefrontParser::ParseObj(path.string());
        for (auto& [name, model] : newModels)
        {
            if (models.count(name) <= 0)
                models[name] = std::move(model);
            else
                LogError(LogType::Resources, "Tried to create model " + name + " multiple times.");
        }
        return;
    }
    if (extension == ".jpg" ||
        extension == ".png")
    {
        if (textures.count(path.string()) <= 0)
            textures[path.string()] = Texture(path.string());
        else
            LogError(LogType::Resources, "Tried to create " + path.string() + " multiple times.");
        return;
    }
}

Model* Engine::GetModel(const std::string& name)
{
    if (models.count(name) > 0)
        return &models[name];
    return nullptr;
}

Texture* Engine::GetTexture(const std::string& name)
{
    if (textures.count(name) > 0)
        return &textures[name];
    return nullptr;
}

void Engine::ResizeCamera(const int& width, const int& height) const
{
    const CameraParams params = camera->GetParams();
    camera->ChangeParams({ width, height, params.near, params.far, params.fov });
}
