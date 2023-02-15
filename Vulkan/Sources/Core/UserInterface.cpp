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

void UserInterface::SetResourceRefs(Camera* _camera, std::vector<Model*>* _models, std::vector<Mesh*>* _meshes, std::vector<Texture*>* _textures)
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
    initInfo.MinImageCount   = renderer->GetVkSwapchainImageCount()-1;
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
    ShowLoaderWindow();
    
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
        ImGui::Text("Vertex count: %lld", app->GetEngine()->GetVertexCount());
        const float deltaTime = app->GetWindow()->GetDeltaTime();
        ImGui::Text("FPS: %d | Delta Time: %.4fs", roundInt(1 / deltaTime), deltaTime);
    }
    ImGui::End();
}

void UserInterface::ShowSceneWindow() const
{
    if (models)
    {
        if (ImGui::Begin("Scene View", NULL, ImGuiWindowFlags_NoMove))
        {
            ImGui::Checkbox("Rotate Models", &app->GetEngine()->rotateModels);
            if (ImGui::TreeNode("Camera"))
            {
                ShowTransformUi(camera->transform);
                static Engine*      engine = app->GetEngine();
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
            for (Model* model : *models)
            {
                if (ImGui::TreeNode(model->GetName()))
                {
                    ShowTransformUi(model->transform);
                    if (ImGui::Button("Remove"))
                        model->shouldDelete = true;
                    ImGui::TreePop();
                }
            }
        }
        ImGui::End();
    }
}

void UserInterface::ShowLoaderWindow() const
{
    if (meshes && textures)
    {
        if (ImGui::Begin("Loader", NULL, ImGuiWindowFlags_NoMove))
        {
            if (ImGui::Button("Load Texture")) {
                ImGui::OpenPopup("TextureLoader");
            }
            if (ImGui::BeginPopup("TextureLoader"))
            {
                static std::string filename;
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Path to texture:");
                ImGui::SameLine();
                ImGui::InputText("##FilenameInput", &filename);
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    textures->push_back(new Texture(filename.c_str()));
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            
            if (ImGui::Button("Load Mesh")) {
                ImGui::OpenPopup("MeshLoader");
            }
            if (ImGui::BeginPopup("MeshLoader"))
            {
                static std::string filename;
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Path to obj:");
                ImGui::SameLine();
                ImGui::InputText("##FilenameInput", &filename);
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    meshes->push_back(new Mesh(filename.c_str()));
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            
            if (ImGui::Button("Load Model")) {
                ImGui::OpenPopup("ModelLoader");
            }
            if (ImGui::BeginPopup("ModelLoader"))
            {
                static std::string name;
                ImGui::InputText("Model name", &name);
                
                static size_t meshIdx = 0;
                if (ImGui::BeginCombo("Mesh", (*meshes)[meshIdx]->GetFilename())) {
                    for (size_t i = 0; i < meshes->size(); i++) {
                        const bool isSelected = (meshIdx == i);
                        if (ImGui::Selectable((*meshes)[i]->GetFilename(), isSelected))
                            meshIdx = i;
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                
                static size_t texIdx = 0;
                if (ImGui::BeginCombo("Texture", (*textures)[texIdx]->GetFilename())) {
                    for (size_t i = 0; i < textures->size(); i++) {
                        const bool isSelected = (texIdx == i);
                        if (ImGui::Selectable((*textures)[i]->GetFilename(), isSelected))
                            texIdx = i;
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                
                if (ImGui::Button("Load")) {
                    models->push_back(new Model(name.c_str(), (*meshes)[meshIdx], (*textures)[texIdx]));
                    app->GetEngine()->UpdateVertexCount();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }
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
