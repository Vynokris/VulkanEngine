#include "Core/Engine.h"
#include "Core/Application.h"
#include "Resources/Camera.h"
#include "Resources/Model.h"
#include "Resources/Mesh.h"
#include "Resources/Texture.h"
#include <filesystem>
#include <fstream>
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
    for (const auto& [name, model]   : models  ) delete model;
    for (const auto& [name, mesh]    : meshes  ) delete mesh;
    for (const auto& [name, texture] : textures) delete texture;
    models  .clear();
    meshes  .clear();
    textures.clear();
    delete camera;
}

void Engine::Awake()
{
    // Create camera.
    camera = new Camera({ app->GetWindow()->GetWidth(), app->GetWindow()->GetHeight(), 0, 1, 60 });

    // Load the default scene.
    LoadScene("Resources\\Scenes\\default.scene");
    
    // Update the vertex count and set the UI's resource pointers.
    UpdateVertexCount();
    app->GetUi()->SetResourceRefs(camera, &models, &meshes, &textures);
}

void Engine::Start() const
{
    // Set camera transform.
    camera->transform.Move({ 0, -1.f, 1.f });
    camera->transform.Rotate(Quaternion::FromPitch(PI * 0.2f));
}

void Engine::Update(const float& deltaTime)
{
    UnloadOutdatedResources();
    if (!sceneToLoad.empty())
    {
        LoadScene(sceneToLoad);
        sceneToLoad = "";
    }
    
    // Update model transforms.
    if (rotateModels)
    {
        const Quaternion rotQuat = Quaternion::FromRoll(PI * 0.2f * deltaTime);
        for (const auto& [name, model] : models)
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
    for (const auto& [name, model] : models)
        renderer->DrawModel(model, camera);
}

void Engine::LoadFile(const std::string& filename)
{
    const fs::path    path = fs::relative(filename);
    const std::string extension = path.extension().string();
    if (extension == ".obj")
    {
        if (meshes.count(path.string()) <= 0)
            meshes[path.string()] = new Mesh(path.string());
        else
            std::cout << "WARNING (Resources): Tried to create " << path.string() << " multiple times." << std::endl;
        return;
    }
    if (extension == ".jpg" ||
        extension == ".png")
    {
        if (textures.count(path.string()) <= 0)
            textures[path.string()] = new Texture(path.string());
        else
            std::cout << "WARNING (Resources): Tried to create " << path.string() << " multiple times." << std::endl;
        return;
    }
    if (extension == ".scene")
    {
        QueueSceneLoad(path.string());
        return;
    }
}

void Engine::LoadScene(const std::string& filename)
{
    UnloadScene();
    sceneName = filename;
    
    // Read file contents and extract them to the data string.
    std::stringstream fileContents;
    {
        std::fstream f(filename, std::ios_base::in | std::ios_base::app);
        fileContents << f.rdbuf();
        f.close();
    }

    std::string line;
    while (std::getline(fileContents, line))
    {
        const char lineType = line[0];
        line = line.substr(2, line.size()-2);
        
        switch (lineType)
        {
        case 'a':
        {
            LoadFile(line);
            break;
        }
        case 'o':
        {
            const size_t nameEnd = line.find(' ');
            const size_t meshEnd = line.find(' ', nameEnd+1);
            const std::string modelName = line.substr(0, nameEnd);
            const std::string meshName  = line.substr(nameEnd+1, meshEnd-nameEnd-1);
            const std::string texName   = line.substr(meshEnd+1, line.size()-meshEnd-1);
            models[modelName] = new Model(modelName, meshes[meshName], textures[texName]);
            break;
        }
        case 't':
        {
            const size_t nameEnd = line.find(' ');
            const size_t xEnd = line.find(' ', nameEnd+1);
            const size_t yEnd = line.find(' ', xEnd+1);
            const std::string modelName = line.substr(0, nameEnd);
            const float xVal = std::stof(line.substr(nameEnd, xEnd-nameEnd));
            const float yVal = std::stof(line.substr(xEnd, yEnd-xEnd));
            const float zVal = std::stof(line.substr(yEnd, line.size()-yEnd));
            models[modelName]->transform.SetPosition({ xVal, yVal, zVal });
            break;
        }
        case 'r':
        {
            const size_t nameEnd = line.find(' ');
            const size_t wEnd = line.find(' ', nameEnd+1);
            const size_t xEnd = line.find(' ', wEnd+1);
            const size_t yEnd = line.find(' ', xEnd+1);
            const std::string modelName = line.substr(0, nameEnd);
            const float wVal = std::stof(line.substr(nameEnd, wEnd-nameEnd));
            const float xVal = std::stof(line.substr(wEnd, xEnd-wEnd));
            const float yVal = std::stof(line.substr(xEnd, yEnd-xEnd));
            const float zVal = std::stof(line.substr(yEnd, line.size()-yEnd));
            models[modelName]->transform.SetRotation({ wVal, xVal, yVal, zVal });
            break;
        }
        case 's':
        {
            const size_t nameEnd = line.find(' ');
            const size_t xEnd = line.find(' ', nameEnd+1);
            const size_t yEnd = line.find(' ', xEnd+1);
            const std::string modelName = line.substr(0, nameEnd);
            const float xVal = std::stof(line.substr(nameEnd, xEnd-nameEnd));
            const float yVal = std::stof(line.substr(xEnd, yEnd-xEnd));
            const float zVal = std::stof(line.substr(yEnd, line.size()-yEnd));
            models[modelName]->transform.SetScale({ xVal, yVal, zVal });
            break;
        }
        default:
            break;
        }
    }
}

void Engine::SaveScene(const std::string& filename) const
{
    // Fill a string with the contents of the scene file.
    std::stringstream fileContents;
    for (const auto& [name, texture] : textures) {
        if (std::count(defaultResources.begin(), defaultResources.end(), name) > 0)
            continue;
        fileContents << "a " << name << std::endl;
    }
    for (const auto& [name, mesh] : meshes) {
        if (std::count(defaultResources.begin(), defaultResources.end(), name) > 0)
            continue;
        fileContents << "a " << name << std::endl;
    }
    for (const auto& [name, model] : models) {
        const Vector3    pos   = model->transform.GetPosition();
        const Quaternion rot   = model->transform.GetRotation();
        const Vector3    scale = model->transform.GetScale();
        fileContents << "o " << name << " " << model->GetMesh()->GetName() << " " << model->GetTexture()->GetName() << std::endl;
        fileContents << "t " << name << " " << pos.x << " " << pos.y << " " << pos.z << std::endl;
        fileContents << "r " << name << " " << rot.w << " " << rot.x << " " << rot.y << " " << rot.z << std::endl;
        fileContents << "s " << name << " " << scale.x << " " << scale.y << " " << scale.z << std::endl;
    }

    // Write the string to the scene file.
    std::fstream f(filename, std::ios_base::out);
    fileContents.seekg(0, std::ios::end);
    f.write(fileContents.str().c_str(), fileContents.tellg());
    f.close();
}

void Engine::UnloadScene()
{
    app->GetRenderer()->WaitUntilIdle();
    for (const auto& [name, model]   : models  ) delete model;
    for (const auto& [name, mesh]    : meshes  ) delete mesh;
    for (const auto& [name, texture] : textures) delete texture;
    models  .clear();
    meshes  .clear();
    textures.clear();
    sceneName = "";
    
    // Load the default resources.
    for (const std::string& resourceName : defaultResources)
        LoadFile(resourceName);
}

void Engine::ResizeCamera(const int& width, const int& height) const
{
    const CameraParams params = camera->GetParams();
    camera->ChangeParams({ width, height, params.near, params.far, params.fov });
}

void Engine::UpdateVertexCount()
{
    vertexCount = 0;
    for (const auto& [name, model] : models)
        vertexCount += model->GetMesh()->GetIndexCount();
}

void Engine::UnloadOutdatedResources()
{
    bool wasModelRemoved = false;
    for (auto it = models.begin(); it != models.end();)
    {
        // Remove outdated models.
        if (it->second->shouldDelete)
        {
            app->GetRenderer()->WaitUntilIdle();
            delete it->second;
            it = models.erase(it);
            wasModelRemoved = true;
            continue;
        }

        // Remove references to outdated meshes.
        if (it->second->GetMesh()->shouldDelete)
        {
            app->GetRenderer()->WaitUntilIdle();
            
            std::string     modelName = it->second->GetName();
            Mesh*           modelMesh = meshes["Resources\\Meshes\\Cube.obj"];
            Texture*        modelTex  = it->second->GetTexture();
            const Transform transform = it->second->transform;
            
            delete it->second;
            it = models.erase(it);
            wasModelRemoved = true;
            models[modelName] = new Model(modelName, modelMesh, modelTex, transform);
            continue;
        }

        // Remove references to outdated textures.
        if (it->second->GetTexture()->shouldDelete)
        {
            app->GetRenderer()->WaitUntilIdle();
            
            std::string     modelName = it->second->GetName();
            Mesh*           modelMesh = it->second->GetMesh();
            Texture*        modelTex  = textures["Resources\\Textures\\Default.png"];
            const Transform transform = it->second->transform;
            
            delete it->second;
            it = models.erase(it);
            models[modelName] = new Model(modelName, modelMesh, modelTex, transform);
            continue;
        }
        
        ++it;
    }
    if (wasModelRemoved) {
        UpdateVertexCount();
    }

    // Remove outdated meshes and textures.
    for (auto it = meshes.begin(); it != meshes.end();) {
        if (it->second->shouldDelete) {
            app->GetRenderer()->WaitUntilIdle();
            delete it->second;
            it = meshes.erase(it);
            continue;
        }
        ++it;
    }
    for (auto it = textures.begin(); it != textures.end();) {
        if (it->second->shouldDelete) {
            app->GetRenderer()->WaitUntilIdle();
            delete it->second;
            it = textures.erase(it);
            continue;
        }
        ++it;
    }
}
