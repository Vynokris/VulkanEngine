#include "Core/Engine.h"
#include "Core/Application.h"
#include "Core/Window.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "Core/RendererShadows.h"
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
#include <cstdarg>
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
    // lights.emplace_back(Light::Directional(Vector3(-1, -1, -1).GetNormalized(), RGBA(1, 1.8f)));
    // lights.emplace_back(Light::Spot(Vector3(1, 1, 1).GetNormalized(), Vector3(-1, -1, -1).GetNormalized(), RGBA(1), 10, 4, 0.1f, 0.05f));
    lights.emplace_back(Light::Point(Vector3(0), RGBA(1, 2), 8, 4));
    Light::UpdateBufferData(lights);
}

void Engine::Start()
{
    // Set camera transform.
    camera->transform.Move({ 0, .5f, 1 });
    camera->transform.Rotate(Quaternion::FromPitch(-PI * 0.1f));

    // Model& model = models.begin()->second;
    // model.transform.Scale({.05f});
    // model.transform.RotateEuler({ -PI/2.5f, 0, 0 });
    // model.transform.Move({ 0, -.5f, -2 });

    // model.GetMeshes()[0].SetMaterial(&materials.begin()->second);
    // model.transform.RotateEuler({ 0,PI,0 });
    // models.at("model_Sphere").GetMeshes()[0].SetMaterial(&materials.begin()->second);
    // models.at("model_Sphere").transform.Move({ 1.5f, 1, 0 });
    // models.at("model_Sphere").transform.RotateEuler({ 0, -PIDIV2-PIDIV4*.5f, 0 });
    // models.at("model_Cube").GetMeshes()[0].SetMaterial(&materials.at("mt_GothicSculptedWall"));
    // models.at("model_Cube").transform.Move({ -1.5f, 1, 0 });

    models.at("model_Cube").GetMeshes()[0].SetMaterial(&materials.at("mt_OilyTubes"));
    models.at("model_Cube").transform.SetScale({ .25f });
    models.at("model_Quad").GetMeshes()[0].SetMaterial(&materials.at("mt_ShadedBasic"));
    models.at("model_Quad").transform.RotateEuler({ PIDIV2, 0, 0 });
    models.at("model_Quad").transform.Move(-Vector3::Up() * 2);
    models.at("model_Quad").transform.SetScale({ 30 });
}

void Engine::Update(const float& deltaTime)
{
    static float time = 0;
    lights.front().direction = Vector3(-cos(time), -1, -sin(time)).GetNormalized();
    lights.front().position  = -lights.front().direction;
    // models.at("model_Cube").transform.SetPosition(Vector3(-cos(time)*2, -1, -sin(time)*2));
    time += deltaTime;
    
    // Update camera transform.
    const WindowInputs inputs = app->GetWindow()->GetInputs();
    if (inputs.mouseRightClick)
    {
        camera->transform.Move(camera->transform.GetRotation().RotateVec(inputs.dirMovement * cameraSpeed * deltaTime));
        camera->transform.Rotate(Quaternion::FromAngleAxis({ -inputs.mouseDelta.y * cameraSensitivity, camera->transform.Right() }));
        camera->transform.Rotate(Quaternion::FromRoll     (  -inputs.mouseDelta.x * cameraSensitivity ));
    }
}

void Engine::Render(Renderer* renderer) const
{
    // Set the viewPos constant in the fragment shader.
    const Vector3 viewPos = camera->transform.GetPosition();
    renderer->SetShaderFrameConstants<GraphicsUtils::ShaderStage::Fragment>({ viewPos });
    Light::UpdateBufferData(lights);

    // Draw all loaded models.
    for (const auto& [name, model] : models)
        renderer->DrawModel(model, *camera);
}

void Engine::Render(const RendererShadows* renderer) const
{
    // Draw all loaded models.
    for (const auto& [name, model] : models)
        renderer->DrawModel(model);
}

void Engine::LoadFile(const std::string& filename, int additionalParamsCount, ...)
{
    va_list args;
    va_start(args, additionalParamsCount);
    
    const fs::path    path = fs::relative(filename);
    const std::string extension = path.extension().string();
    if (extension == ".obj")
    {
        std::unordered_map<std::string, Model> newModels = WavefrontParser::ParseObj(path.string());
        for (auto& [name, model] : newModels)
        {
            if (models.find(name) == models.end())
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
            if (materials.find(name) == materials.end())
                materials[name] = std::move(material);
            else
                LogWarning(LogType::Resources, "Tried to create material " + name + " multiple times.");
        }
        return;
    }
    if (extension == ".jpg" ||
        extension == ".png" ||
        extension == ".jpeg")
    {
        const std::string pathStr = path.string();
        if (textures.find(pathStr) == textures.end())
            textures[pathStr] = Texture(pathStr, additionalParamsCount > 0 ? va_arg(args, bool) : true);
        else
            LogWarning(LogType::Resources, "Tried to create " + path.string() + " multiple times.");
        return;
    }
}

Light* Engine::GetLight(const size_t& idx)
{
    if (idx < lights.size())
        return &lights[idx];
    return nullptr;
}

Model* Engine::GetModel(const std::string& name)
{
    if (models.find(name) != models.end())
        return &models[name];
    return nullptr;
}

Material* Engine::GetMaterial(const std::string& name)
{
    if (materials.find(name) != materials.end())
        return &materials[name];
    return nullptr;
}

Texture* Engine::GetTexture(const std::string& name)
{
    if (textures.find(name) != textures.end())
        return &textures[name];
    return nullptr;
}

void Engine::ResizeCamera(const int& width, const int& height) const
{
    const CameraParams params = camera->GetParams();
    camera->SetParams({ width, height, params.near, params.far, params.fov });
}
