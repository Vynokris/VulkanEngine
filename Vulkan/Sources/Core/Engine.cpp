#include "Core/Engine.h"
#include "Core/Application.h"
#include "Core/Window.h"
#include "Core/Logger.h"
#include "Core/Renderer.h"
#include "Core/UserInterface.h"
#include "Maths/MathConstants.h"
#include "Maths/AngleAxis.h"
#include "Resources/Camera.h"
#include "Resources/Mesh.h"
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
    // Create camera and init distance fog params.
    const float camNear = 0.1f, camFar = 100, camFov = 80;
    const float fogStart = camFar * 0.6f;
    camera = new Camera({ app->GetWindow()->GetWidth(), app->GetWindow()->GetHeight(), camNear, camFar, camFov });
    app->GetRenderer()->SetDistanceFogParams(0, fogStart, camFar);
    
    // Update the vertex count and set the UI's resource pointers.
    app->GetUi()->SetResourceRefs(camera, &models, &textures);

    // Load default resources.
    for (const std::string& filename : defaultResources)
        LoadFile(filename);

    // Add default directional light.
    // lights.emplace_back(Light::Directional(Vector3(-1, -1, -1).GetNormalized(), RGBA(1, 1.8f)));
    // lights.emplace_back(Light::Spot(Vector3(1, 1, 1).GetNormalized(), Vector3(-1, -1, -1).GetNormalized(), RGBA(1), 10, 4, 0.1f, 0.05f));
    // lights.emplace_back(Light::Point(Vector3(0), RGBA(1, 2), 8, 4));
    // Light::UpdateBufferData(lights);
}

void Engine::Start()
{
    // Set camera transform.
    camera->transform.Move({ 0, .5f, 1 });
    camera->transform.Rotate(Quaternion::FromPitch(-PI * 0.1f));

    Model& model = models.begin()->second;
    model.transforms.front().Scale({.1f});
    model.transforms.front().RotateEuler({ PIDIV2, 0, 0 });
    // model.transforms.front().RotateEuler({ 0, PIDIV2, 0 });
    // model.transforms.front().Move({ 0, -.5f, -2 });

    // model.GetMeshes()[0].SetMaterial(&materials.begin()->second);
    // model.transform.RotateEuler({ 0,PI,0 });
    // models.at("model_Sphere").GetMeshes()[0].SetMaterial(&materials.begin()->second);
    // models.at("model_Sphere").transform.Move({ 1.5f, 1, 0 });
    // models.at("model_Sphere").transform.RotateEuler({ 0, -PIDIV2-PIDIV4*.5f, 0 });
    // models.at("model_Cube").GetMeshes()[0].SetMaterial(&materials.at("mt_GothicSculptedWall"));
    // models.at("model_Cube").transform.Move({ -1.5f, 1, 0 });

    // models.at("model_Cube").GetMeshes()[0].SetMaterial(&materials.at("mt_OilyTubes"));
    // models.at("model_Cube").transforms.front().SetScale({ .25f });
    // materials.at("mt_WornPavement").depthMultiplier = 0.01f;
    // models.at("model_Quad").GetMeshes()[0].SetMaterial(&materials.at("mt_WornPavement"));
    // models.at("model_Quad").transform.RotateEuler({ PIDIV2, 0, 0 });
    // models.at("model_Quad").transform.Move(-Vector3::Up() * 2);
    // models.at("model_Quad").transform.SetScale({ 30 });

    // lights.front().direction = Vector3(0, -1, -1).GetNormalized();
    // lights.front().position  = -lights.front().direction;
}

void Engine::Update(const float& deltaTime)
{
    static float time = 0;
    // lights.front().direction = Vector3(-cos(time), -sin(time), -1).GetNormalized();
    // lights.front().position  = -lights.front().direction;
    // lights.front().position = Vector3(0, 2, cos(time / 3) * 3);
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

void Engine::Render(Renderer* renderer)
{
    // Set the viewProj and viewPos constants in the shaders.
    const GraphicsUtils::ShaderFrameConstants frameConstants = {
        camera->GetViewMat() * camera->GetProjMat(),
        camera->transform.GetPosition()
    };
    renderer->SetShaderFrameConstants(frameConstants);
    Light::UpdateBufferData(lights);

    // Draw all loaded models.
    const Cubemap* cubemap = GetMainCubemap();
    for (const auto& [name, model] : models)
        renderer->DrawModel(model, cubemap);
}

static bool IsTextureFile(const std::string& extension)
{
    return extension == ".jpg"  ||
           extension == ".png"  ||
           extension == ".jpeg" ||
           extension == ".tga";
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
    if (IsTextureFile(extension))
    {
        const std::string pathStr = path.string();
        if (textures.find(pathStr) == textures.end())
            textures[pathStr] = Texture(pathStr, additionalParamsCount > 0 ? va_arg(args, bool) : true);
        else
            LogWarning(LogType::Resources, "Tried to create " + path.string() + " multiple times.");
        return;
    }
    if (filename.back() == '\\')
    {
        const std::string pathStr = path.string();
        uint32_t texCount = 0;
        std::array<std::string, 6> texPaths;
        for (const auto& entry : fs::directory_iterator(path))
        {
            if (!IsTextureFile(entry.path().extension().string()))
                continue;
            if (texCount < 6)
                texPaths[texCount] = entry.path().string();
            ++texCount;
        }
        if (texCount == 6)
        {
            if (cubemaps.find(pathStr) == cubemaps.end())
                cubemaps[pathStr] = Cubemap(texPaths);
            else
                LogWarning(LogType::Resources, "Tried to create " + path.string() + " multiple times.");
        }
        else
            LogWarning(LogType::Resources, "Unable to load cubemap, only found " + std::to_string(texPaths.size()) + " textures in directory " + path.string());
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

Cubemap* Engine::GetMainCubemap()
{
    if (cubemaps.empty())
        return nullptr;
    return &cubemaps.begin()->second;
}

void Engine::ResizeCamera(const int& width, const int& height) const
{
    const CameraParams params = camera->GetParams();
    camera->SetParams({ width, height, params.near, params.far, params.fov });
}
