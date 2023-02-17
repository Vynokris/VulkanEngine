#include "Core/UserInterface.h"
#include "Core/Application.h"
#include "Resources/Camera.h"
#include "Resources/Model.h"
#include "Resources/Mesh.h"
#include "Resources/Texture.h"
#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
#include <vulkan/vulkan_core.h>
using namespace Core;
using namespace Resources;
using namespace Maths;

UserInterface::UserInterface()
{
    app = Application::Get();
    engine = app->GetEngine();
    
    CreateDescriptorPool();
    InitImGui();
    UploadImGuiFonts();
}

UserInterface::~UserInterface()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(app->GetRenderer()->GetVkDevice(), vkDescriptorPool, nullptr);
}

void UserInterface::SetResourceRefs(Camera* _camera, std::unordered_map<std::string, Model*>* _models, std::unordered_map<std::string, Mesh*>* _meshes, std::unordered_map<std::string, Texture*>* _textures)
{
    camera   = _camera;
    models   = _models;
    meshes   = _meshes;
    textures = _textures;
}

void UserInterface::CreateDescriptorPool()
{
    // Define the descriptor pool sizes.
    const VkDescriptorPoolSize poolSizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER,                1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000 }
    };

    // Create the descriptor pool.
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets       = 1000 * IM_ARRAYSIZE(poolSizes);
    poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
    poolInfo.pPoolSizes    = poolSizes;
    if (vkCreateDescriptorPool(app->GetRenderer()->GetVkDevice(), &poolInfo, nullptr, &vkDescriptorPool) != VK_SUCCESS) {
        std::cout << "ERROR (Vulkan): Failed to create a descriptor pool." << std::endl;
        throw std::runtime_error("VULKAN_DESCRIPTOR_POOL_ERROR");
    }
}

void UserInterface::InitImGui() const
{
    const Renderer* renderer = app->GetRenderer();
    
    // Create ImGui context.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    // Setup platform/renderer backends.
    ImGui_ImplGlfw_InitForVulkan(app->GetWindow()->GetGlfwWindow(), true);
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance        = renderer->GetVkInstance();
    initInfo.PhysicalDevice  = renderer->GetVkPhysicalDevice();
    initInfo.Device          = renderer->GetVkDevice();
    initInfo.QueueFamily     = renderer->GetVkGraphicsQueueIndex();
    initInfo.Queue           = renderer->GetVkGraphicsQueue();
    initInfo.PipelineCache   = VK_NULL_HANDLE;
    initInfo.DescriptorPool  = vkDescriptorPool;
    initInfo.Subpass         = 0;
    initInfo.MinImageCount   = renderer->GetVkSwapchainImageCount();
    initInfo.ImageCount      = renderer->GetVkSwapchainImageCount();
    initInfo.MSAASamples     = renderer->GetMsaaSamples();
    initInfo.Allocator       = nullptr;
    initInfo.CheckVkResultFn = [](VkResult res){ if (res==0) return; std::cout << "Error (Vulkan): result = " << res << std::endl; throw std::runtime_error("VULKAN_ERROR"); };
    ImGui_ImplVulkan_Init(&initInfo, renderer->GetVkRenderPass());
}

void UserInterface::UploadImGuiFonts() const
{
    // Use any command queue
    const Renderer*       renderer        = app->GetRenderer();
    const VkDevice        vkDevice        = renderer->GetVkDevice();
    const VkCommandPool   vkCommandPool   = renderer->GetVkCommandPool();
    const VkCommandBuffer vkCommandBuffer = VulkanUtils::BeginSingleTimeCommands(vkDevice, vkCommandPool);
    ImGui_ImplVulkan_CreateFontsTexture(vkCommandBuffer);
    VulkanUtils::EndSingleTimeCommands(vkDevice, vkCommandPool, renderer->GetVkGraphicsQueue(), vkCommandBuffer);
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void UserInterface::Render() const
{
    NewFrame();
    
    ShowStatsWindow();
    ShowSceneWindow();
    ShowResourcesWindow();
    
    RenderFrame();
}

void UserInterface::ShowTransformUi(Maths::Transform& transform) const
{
    // Inputs for position.
    Vector3 objectPos = transform.GetPosition(); objectPos.y *= -1;
    if (ImGui::DragFloat3("Position", &objectPos.x, 0.1f))
        transform.SetPosition({ objectPos.x, -objectPos.y, objectPos.z });

    // Inputs for rotation.
    Quaternion objectRot = transform.GetRotation();
    if (ImGui::DragFloat4("Rotation", &objectRot.w, 0.1f))
        transform.SetRotation(objectRot);

    // Inputs for scale.
    if (!transform.IsCamera())
    {
        Vector3 objectScale = transform.GetScale();
        if (ImGui::DragFloat3("Scale", &objectScale.x, 0.05f))
            transform.SetScale(objectScale);
    }
}

void UserInterface::ShowStatsWindow() const
{
    if (ImGui::Begin("Stats", NULL, ImGuiWindowFlags_NoMove))
    {
        ImGui::Text("Vertex count: %lld", engine->GetVertexCount());
        const float deltaTime = app->GetWindow()->GetDeltaTime();
        ImGui::Text("FPS: %d | Delta Time: %.4fs", roundInt(1 / deltaTime), deltaTime);
    }
    ImGui::End();
}

void UserInterface::ShowSceneWindow() const
{
    if (ImGui::Begin("Scene View", NULL, ImGuiWindowFlags_NoMove) && models)
    {
        // Show loaded scene.
        ImGui::Text("Loaded scene: %s", engine->GetSceneName().c_str());

        // New scene button.
        if (ImGui::Button("New Scene")) {
            ImGui::OpenPopup("SceneCreation");
        }
        if (ImGui::BeginPopup("SceneCreation"))
        {
            static std::string filename = "Resources\\Scenes\\";
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Scene filename");
            ImGui::SameLine();
            ImGui::InputText("##FilenameInput", &filename);
            ImGui::SameLine();
            if (ImGui::Button("Create")) {
                engine->QueueSceneLoad(filename);
                filename = "Resources\\Scenes\\";
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }

        // Save scene button.
        ImGui::SameLine();
        if (ImGui::Button("Save Scene"))
            engine->SaveScene(engine->GetSceneName());

        // Add model button.
        ImGui::SameLine();
        if (ImGui::Button("Add Model") && !meshes->empty() && !textures->empty()) {
            ImGui::OpenPopup("ModelLoader");
        }
        if (ImGui::BeginPopup("ModelLoader"))
        {
            // Input for model name.
            static std::string modelName;
            ImGui::InputText("Model name", &modelName);

            // Input for model mesh.
            static std::string meshName = meshes->begin()->first;
            if (ImGui::BeginCombo("Mesh", meshName.c_str())) {
                for (auto it = meshes->begin(); it != meshes->end(); ++it) {
                    const bool isSelected = (meshName == it->first);
                    if (ImGui::Selectable(it->first.c_str(), isSelected))
                        meshName = it->first;
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            // Input for model texture.
            static std::string texName = textures->begin()->first;
            if (ImGui::BeginCombo("Texture", texName.c_str())) {
                for (auto it = textures->begin(); it != textures->end(); ++it) {
                    const bool isSelected = (texName == it->first);
                    if (ImGui::Selectable(it->first.c_str(), isSelected))
                        texName = it->first;
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            // Button to load model.
            if (ImGui::Button("Load") && !modelName.empty()) {
                (*models)[modelName] = new Model(modelName, (*meshes)[meshName], (*textures)[texName]);
                modelName = "";
                app->GetEngine()->UpdateVertexCount();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Checkbox to rotate model automatically.
        ImGui::Checkbox("Rotate Models", &engine->rotateModels);

        // Inputs for camera configuration.
        if (ImGui::TreeNode("Camera"))
        {
            ShowTransformUi(camera->transform);
            static CameraParams params = camera->GetParams();
            if (ImGui::DragFloat2("Near/Far", &params.near, 0.1f, 0)) {
                params.near = clampAbove(params.near, 0);
                params.far  = clampAbove(params.far, 0);
                camera->ChangeParams(params);
            }
            if (ImGui::DragFloat("FOV", &params.fov, 0.1f, 0, 180)) {
                params.fov = clamp(params.fov, 0, 180);
                camera->ChangeParams(params);
            }
            ImGui::DragFloat("Speed", &engine->cameraSpeed, 0.1f);
            ImGui::DragFloat("Sensitivity", &engine->cameraSensitivity, 0.1f);
            ImGui::TreePop();
        }

        // Inputs for models.
        for (const auto& [name, model] : *models)
        {
            const bool treeNodeOpen = ImGui::TreeNode(name.c_str());
            
            if (ImGui::BeginPopupContextItem())
            {
                static std::string newName;
                if (ImGui::Button("Rename") && !newName.empty())
                {
                    (*models)[newName] = new Model(newName, model->GetMesh(), model->GetTexture(), model->transform);
                    model->shouldDelete = true;
                    newName = "";
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(120);
                ImGui::InputText("##NameInput", &newName);
                if (ImGui::Button("Remove")) {
                    model->shouldDelete = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            if (treeNodeOpen)
            {
                ShowTransformUi(model->transform);
                ImGui::TreePop();
            }
        }
    }
    ImGui::End();
}

void UserInterface::ShowResourcesWindow() const
{
    if (ImGui::Begin("Resource Viewer", NULL, ImGuiWindowFlags_NoMove))
    {
        if (ImGui::TreeNode("Meshes"))
        {
            ImGui::Indent(7);
            for (const auto& [name, mesh] : *meshes)
            {
                if (std::count(engine->defaultResources.begin(), engine->defaultResources.end(), name) > 0)
                    continue;

                ImGui::Selectable(name.c_str());
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::Button(("Unload##Mesh"+name).c_str())) {
                        mesh->shouldDelete = true;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::Unindent(7);
            ImGui::TreePop();
        }

        ImGui::AlignTextToFramePadding();
        if (ImGui::TreeNode("Textures"))
        {
            ImGui::Indent(7);
            for (const auto& [name, texture] : *textures)
            {
                if (std::count(engine->defaultResources.begin(), engine->defaultResources.end(), name) > 0)
                    continue;
                
                ImGui::Selectable(name.c_str());
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::Button(("Unload##Tex"+name).c_str())) {
                        texture->shouldDelete = true;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::Unindent(7);
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

void UserInterface::NewFrame() const
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UserInterface::RenderFrame() const
{
    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(drawData, app->GetRenderer()->GetCurVkCommandBuffer());
}
