#include "Core/Engine.h"
#include "Core/Application.h"
#include "Core/Window.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "Core/UserInterface.h"
#include "Maths/MathConstants.h"
#include "Maths/AngleAxis.h"
#include "Resources/Camera.h"
#include "Resources/Model.h"
#include "Resources/Mesh.h"
#include "Resources/Texture.h"
#include "Core/WavefrontParser.h"
#include "Resources/Light.h"
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;
using namespace Core;
using namespace Resources;
using namespace Maths;

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

    // Add default directional light.
    // lights.emplace_back(Light::Directional({1,1,1}, Vector3(-1, -1, -1).GetNormalized(), 1));
    lights.emplace_back(Light::Point({1,1,1}, {0,2,2}, 1, 8, 4));
    // lights.emplace_back(Light::Spot({1,1,1}, {3,3,2}, Vector3(-3,-3,-2).GetNormalized(), 1, 10, 4, 0.1f, 0.05f));
    Light::UpdateBufferData(lights);
}

void Engine::Start()
{
    // Set camera transform.
    camera->transform.Move({ 0, 2.f, 2.f });
    camera->transform.Rotate(Quaternion::FromPitch(-PI * 0.2f));

    models.at("model_Sphere").GetMeshes()[0].SetMaterial(&materials.at("mt_GothicSculptedWall"));
    models.at("model_Sphere").transform.Move({ 1.5f, 1, 0 });
    models.at("model_Cube").GetMeshes()[0].SetMaterial(&materials.at("mt_GothicSculptedWall"));
    models.at("model_Cube").transform.Move({ -1.5f, 1, 0 });
}

void Engine::Update(const float& deltaTime)
{
    // Update camera transform.
    const WindowInputs inputs = app->GetWindow()->GetInputs();
    if (inputs.mouseRightClick)
    {
        camera->transform.Move(camera->transform.GetRotation().RotateVec(inputs.dirMovement * cameraSpeed * deltaTime));
        camera->transform.Rotate(Quaternion::FromAngleAxis({ -inputs.mouseDelta.y * cameraSensitivity, camera->transform.Right() }));
        camera->transform.Rotate(Quaternion::FromRoll     (  -inputs.mouseDelta.x * cameraSensitivity ));
    }
}

void Engine::Render(const Renderer* renderer) const
{
    // Set the viewPos in the fragment shader.
    // TODO: Move this to the Renderer.
    const Vector3 viewPos = camera->transform.GetPosition();
    vkCmdPushConstants(renderer->GetCurVkCommandBuffer(), renderer->GetVkPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Vector3), &viewPos);

    for (const auto& [name, model] : models)
        renderer->DrawModel(model, *camera);
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
                LogWarning(LogType::Resources, "Tried to create model " + name + " multiple times.");
        }
        return;
    }
    if (extension == ".mtl")
    {
        std::unordered_map<std::string, Material> newMaterials = WavefrontParser::ParseMtl(path.string());
        for (auto& [name, material] : newMaterials)
        {
            if (materials.count(name) <= 0)
                materials[name] = std::move(material);
            else
                LogWarning(LogType::Resources, "Tried to create material " + name + " multiple times.");
        }
        return;
    }
    if (extension == ".jpg" ||
        extension == ".png")
    {
        if (textures.count(path.string()) <= 0)
            textures[path.string()] = Texture(path.string());
        else
            LogWarning(LogType::Resources, "Tried to create " + path.string() + " multiple times.");
        return;
    }
}

Model* Engine::GetModel(const std::string& name)
{
    if (models.count(name) > 0)
        return &models[name];
    return nullptr;
}

Material* Engine::GetMaterial(const std::string& name)
{
    if (materials.count(name) > 0)
        return &materials[name];
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
    camera->SetParams({ width, height, params.near, params.far, params.fov });
}
