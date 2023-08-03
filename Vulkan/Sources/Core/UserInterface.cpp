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

void UserInterface::SetResourceRefs(Camera* _camera, std::unordered_map<std::string, Model*>* _models, std::unordered_map<std::string, Texture*>* _textures)
{
    camera   = _camera;
    models   = _models;
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
    ShowResourcesWindow();
    
    RenderFrame();
}

void UserInterface::ShowStatsWindow() const
{
    if (ImGui::Begin("Stats", NULL, ImGuiWindowFlags_NoMove))
    {
        const float deltaTime = app->GetWindow()->GetDeltaTime();
        ImGui::Text("FPS: %d | Delta Time: %.4fs", roundInt(1 / deltaTime), deltaTime);
    }
    ImGui::End();
}

void UserInterface::ShowResourcesWindow() const
{
    if (ImGui::Begin("Resource Viewer", NULL, ImGuiWindowFlags_NoMove))
    {
        if (ImGui::TreeNode("Models"))
        {
            ImGui::Indent(7);
            for (const auto& [name, model] : *models)
            {
                if (std::count(engine->defaultResources.begin(), engine->defaultResources.end(), name) > 0)
                    continue;

                ImGui::Selectable(name.c_str());
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::Button(("Unload##Mesh"+name).c_str())) {
                        // TODO.
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
