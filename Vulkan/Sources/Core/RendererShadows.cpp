#include "Core/RendererShadows.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Core/Window.h"
#include "Core/Renderer.h"
#include "Core/GpuDataManager.h"
#include "Core/Engine.h"
#include "Resources/Camera.h"
#include "Resources/Model.h"
#include "Resources/Mesh.h"
#include "Resources/Light.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <array>
#include <fstream>
using namespace Core;
using namespace GraphicsUtils;

RendererShadows::RendererShadows(Application* application, Renderer* appRenderer, uint32_t shadowMapWidth, uint32_t shadowMapHeight)
    : vkShadowImageWidth(shadowMapWidth), vkShadowImageHeight(shadowMapHeight)
{
    app      = application;
    renderer = appRenderer;
    gpuData  = app->GetGpuData();
    
    vkShadowImageFormat = FindSupportedFormat(renderer->GetVkPhysicalDevice(),
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
    CreateRenderPass();
    CreateTextureSampler();
    CreateShadowImage();
    CreateDescriptors();
    CreatePipeline();
    // CreateCommandPool();
    // CreateCommandBuffers();
    // CreateSyncObjects();
}

RendererShadows::~RendererShadows()
{
    WaitUntilIdle();

    const VkDevice vkDevice = renderer->GetVkDevice();
    // vkDestroySemaphore  (vkDevice, vkRenderFinishedSemaphore, nullptr);
    // vkDestroySemaphore  (vkDevice, vkImageAvailableSemaphore, nullptr);
    // vkDestroyFence      (vkDevice, vkInFlightFence,           nullptr);
    // vkDestroyCommandPool(vkDevice, vkCommandPool,             nullptr);
    vkDestroyPipeline              (vkDevice, vkPipeline,             nullptr);
    vkDestroyPipelineLayout        (vkDevice, vkPipelineLayout,       nullptr);
    vkDestroyBuffer                (vkDevice, vkBuffer,               nullptr);
    vkFreeMemory                   (vkDevice, vkBufferMemory,         nullptr);
    vkDestroyDescriptorPool        (vkDevice, vkDescriptorPool,       nullptr);
    vkDestroyDescriptorSetLayout   (vkDevice, vkDescriptorSetLayout,  nullptr);
    DestroyShadowImage();
    vkDestroySampler               (vkDevice, vkShadowSampler,        nullptr);
    vkDestroyRenderPass            (vkDevice, vkRenderPass,           nullptr);
}

void RendererShadows::BeginRender(const Resources::LightType lightType)
{
    NewFrame();
    BeginRenderPass();
    UpdateViewportScissor(lightType);
}

void RendererShadows::NextRender(const Resources::LightType lightType)
{
    renderIdx++;
    UpdateViewportScissor(lightType);
}

void RendererShadows::DrawModel(const Resources::Model& model) const
{
    // Get the current command buffer.
    const uint32_t currentFrame = renderer->GetCurFramebufferIdx();
    const VkCommandBuffer vkCommandBuffer = renderer->GetCurVkCommandBuffer();
    
    // Get the light array as well as the model's GPU data and update it.
    const GpuData<Resources::Model>& modelData = gpuData->GetData(model);

    // Draw each of the model's meshes one by one.
    const std::vector<Resources::Mesh>& meshes = model.GetMeshes();
    for (const Resources::Mesh& mesh : meshes)
    {
        // Get the mesh and material GPU data.
        const GpuData<Resources::Mesh>& meshData = gpuData->GetData(mesh);
        
        // Bind the vertex and index buffers.
        const VkBuffer vertexBuffer = meshData.vkVertexBuffer;
        const VkBuffer indexBuffer  = meshData.vkIndexBuffer ;
        constexpr VkDeviceSize vertexOffset = 0;
        vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, &vertexBuffer, &vertexOffset);
        vkCmdBindIndexBuffer  (vkCommandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // Bind the descriptor sets and draw.
        const VkDescriptorSet descriptorSets[2] = { modelData.vkDescriptorSets[currentFrame], vkDescriptorSet };
        const uint32_t        dynamicOffset     = renderIdx * sizeof(Maths::Mat4);
        vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineLayout, 0, 2, descriptorSets, 1, &dynamicOffset);
        vkCmdDrawIndexed(vkCommandBuffer, mesh.GetIndexCount(), 1, 0, 0, 0);
    }
}

void RendererShadows::EndRender()
{
    EndRenderPass();
    renderIdx = 0;

    // Resize the depth image if requested.
    if (framebufferResized) {
        framebufferResized = false;
        RecreateShadowImage();
    }
}

void RendererShadows::WaitUntilIdle() const
{
    vkDeviceWaitIdle(renderer->GetVkDevice());
}

#pragma region Setup
void RendererShadows::CreateRenderPass()
{
    // Set the depth buffer attachment.
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format         = vkShadowImageFormat;
    depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 0;
    depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Create a single sub-pass.
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 0;
    subpass.pColorAttachments       = nullptr;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments     = nullptr;

    // Create a sub-pass dependency.
    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT          | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // Set the render pass creation information.
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments    = &depthAttachment;
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependency;

    // Create the render pass.
    if (vkCreateRenderPass(renderer->GetVkDevice(), &renderPassInfo, nullptr, &vkRenderPass) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create Main render pass.");
        throw std::runtime_error("VULKAN_RENDER_PASS_ERROR");
    }
}
void RendererShadows::CreateTextureSampler()
{
    const VkDevice vkDevice = renderer->GetVkDevice();
    
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter               = VK_FILTER_LINEAR;
    samplerInfo.minFilter               = VK_FILTER_LINEAR;
    samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.compareEnable           = VK_TRUE;
    samplerInfo.compareOp               = VK_COMPARE_OP_LESS_OR_EQUAL;
    samplerInfo.anisotropyEnable        = VK_FALSE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.maxAnisotropy           = 1.f;
    samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod                  = 0.f;
    samplerInfo.maxLod                  = 0.f;
    if (vkCreateSampler(vkDevice, &samplerInfo, nullptr, &vkShadowSampler) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create texture sampler.");
        throw std::runtime_error("VULKAN_TEXTURE_SAMPLER_ERROR");
    }
}

void RendererShadows::CreateShadowImage()
{
    const VkDevice         vkDevice         = renderer->GetVkDevice();
    const VkPhysicalDevice vkPhysicalDevice = renderer->GetVkPhysicalDevice();
    
    CreateImage(vkDevice, vkPhysicalDevice, vkShadowImageWidth, vkShadowImageHeight,
                    1, VK_SAMPLE_COUNT_1_BIT, vkShadowImageFormat,
                    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    vkShadowImage, vkShadowImageMemory);
        
    CreateImageView(vkDevice, vkShadowImage, vkShadowImageFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, vkShadowImageView);

    // Create the current framebuffer creation information.
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = vkRenderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments    = &vkShadowImageView;
    framebufferInfo.width           = vkShadowImageWidth;
    framebufferInfo.height          = vkShadowImageHeight;
    framebufferInfo.layers          = 1;

    // Create the shadow framebuffer.
    if (vkCreateFramebuffer(vkDevice, &framebufferInfo, nullptr, &vkShadowFramebuffer) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create framebuffer.");
        throw std::runtime_error("VULKAN_FRAMEBUFFER_ERROR");
    }
}

void RendererShadows::CreateDescriptors()
{
    // Get necessary Vulkan resources.
    const VkDevice         vkDevice         = renderer->GetVkDevice();
    const VkPhysicalDevice vkPhysicalDevice = renderer->GetVkPhysicalDevice();
    
    // Set the bindings for the light matrix, data, and shadow map.
    VkDescriptorSetLayoutBinding layoutBindings[3] = {{}};
    layoutBindings[0].binding         = 0;
    layoutBindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBindings[1].binding         = 1;
    layoutBindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBindings[2].binding         = 2;
    layoutBindings[2].descriptorCount = 1;
    layoutBindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindings[2].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    // Create the descriptor set layout.
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 3;
    layoutInfo.pBindings    = layoutBindings;
    if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &vkDescriptorSetLayout) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create descriptor set layout.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_SET_LAYOUT_ERROR");
    }

    // Set the type and number of descriptors.
    VkDescriptorPoolSize poolSizes[3] = {{}};
    poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = 1;
    poolSizes[2].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[2].descriptorCount = 1;

    // Create the descriptor pool.
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 3;
    poolInfo.pPoolSizes    = poolSizes;
    poolInfo.maxSets       = 1;
    if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &vkDescriptorPool) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create descriptor pool.");
        throw std::runtime_error("VULKAN_DESCRIPTOR_POOL_ERROR");
    }

    // Allocate the descriptor set.
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = vkDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &vkDescriptorSetLayout;
    if (vkAllocateDescriptorSets(vkDevice, &allocInfo, &vkDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    // Create the light matrix buffer.
    vkBufferSize = sizeof(Maths::Mat4) * 4;
    CreateBuffer(vkDevice, vkPhysicalDevice, vkBufferSize,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 vkBuffer, vkBufferMemory);
    vkMapMemory(vkDevice, vkBufferMemory, 0, vkBufferSize, 0, &vkBufferMapped);

    // Define info for the light matrix buffer.
    VkDescriptorBufferInfo bufferInfos[2] = {{}};
    bufferInfos[0].buffer = vkBuffer;
    bufferInfos[0].offset = 0;
    bufferInfos[0].range  = sizeof(Maths::Mat4);
    bufferInfos[1].buffer = vkBuffer;
    bufferInfos[1].offset = 0;
    bufferInfos[1].range  = vkBufferSize;
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView   = vkShadowImageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
    imageInfo.sampler     = vkShadowSampler;

    // Populate the descriptor set.
    VkWriteDescriptorSet descriptorWrites[3] = {{}};
    descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet          = vkDescriptorSet;
    descriptorWrites[0].dstBinding      = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo     = &bufferInfos[0];
    descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet          = vkDescriptorSet;
    descriptorWrites[1].dstBinding      = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo     = &bufferInfos[1];
    descriptorWrites[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet          = vkDescriptorSet;
    descriptorWrites[2].dstBinding      = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pImageInfo      = &imageInfo;
    vkUpdateDescriptorSets(vkDevice, 3, descriptorWrites, 0, nullptr);
}

void RendererShadows::CreatePipeline()
{
    const VkDevice vkDevice = renderer->GetVkDevice();
    
    // Setup vertex bindings and attributes.
    auto bindingDescription    = Resources::Mesh::GetVertexBindingDescription();
    auto attributeDescriptions = Resources::Mesh::GetVertexAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size();
    vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

    // Specify the kind of geometry to be drawn.
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Specify the scissor rectangle (pixels outside it will be discarded).
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { vkShadowImageWidth, vkShadowImageHeight };

    // Create the dynamic states for viewport and scissor.
    std::vector dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
    dynamicState.pDynamicStates    = dynamicStates.data();
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    // Set rasterizer parameters.
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL; // Use VK_POLYGON_MODE_LINE to draw wireframe.
    rasterizer.lineWidth               = 1.f;
    rasterizer.cullMode                = VK_CULL_MODE_FRONT_BIT;
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.f; // Optional.
    rasterizer.depthBiasClamp          = 0.f; // Optional.
    rasterizer.depthBiasSlopeFactor    = 0.f; // Optional.

    // Depth and stencil buffer parameters.
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable       = VK_TRUE;
    depthStencil.depthWriteEnable      = VK_TRUE;
    depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds        = 0.f; // Optional.
    depthStencil.maxDepthBounds        = 1.f; // Optional.
    depthStencil.stencilTestEnable     = VK_FALSE;
    depthStencil.front                 = {}; // Optional.
    depthStencil.back                  = {}; // Optional.

    // Load vulkan fragment and vertex shaders.
    VkShaderModule vertShaderModule = CreateShaderModule(vkDevice, ShaderStage::Vertex,   "Shaders/Depth.vert");
    VkShaderModule fragShaderModule = CreateShaderModule(vkDevice, ShaderStage::Fragment, "Shaders/Depth.frag");

    // Set the vertex shader's pipeline stage and entry point.
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName  = "main";

    // Set the fragment shader's pipeline stage and entry point.
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    // Create an array of both pipeline stage info structures.
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // Set multisampling parameters.
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampling.sampleShadingEnable   = VK_TRUE;
    multisampling.minSampleShading      = .2f;
    multisampling.pSampleMask           = nullptr;  // Optional.
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional.
    multisampling.alphaToOneEnable      = VK_FALSE; // Optional.

    // Define 2 descriptor set layouts (modelMatrix, lightMatrix).
    const VkDescriptorSetLayout setLayouts[2] = {
        gpuData->GetArray<Resources::Model>().vkDescriptorSetLayout,
        vkDescriptorSetLayout
    };

    // Set the pipeline layout creation information.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = 2;
    pipelineLayoutInfo.pSetLayouts            = setLayouts;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges    = nullptr;

    // Create the pipeline layout.
    if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &vkPipelineLayout) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create pipeline layout.");
        throw std::runtime_error("VULKAN_PIPELINE_LAYOUT_ERROR");
    }

    // Set the graphics pipeline creation information.
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = 2;
    pipelineInfo.pStages             = shaderStages;
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = &depthStencil;
    pipelineInfo.pColorBlendState    = nullptr;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.layout              = vkPipelineLayout;
    pipelineInfo.renderPass          = vkRenderPass;
    pipelineInfo.subpass             = 0;

    // Create the graphics pipeline.
    if (vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkPipeline) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create graphics pipeline.");
        throw std::runtime_error("VULKAN_GRAPHICS_PIPELINE_ERROR");
    }

    // Destroy both shader modules.
    vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
    vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
}
/*
void RendererShadows::CreateCommandPool()
{
    const QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(vkPhysicalDevice, vkSurface);

    // Set the command pool creation information.
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    // Create the command pool.
    if (vkCreateCommandPool(vkDevice, &poolInfo, nullptr, &vkCommandPool) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to create command pool.");
        throw std::runtime_error("VULKAN_COMMAND_POOL_ERROR");
    }
}

void RendererShadows::CreateCommandBuffers()
{
    // Set the command buffer allocation information.
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = vkCommandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    // Allocate the command buffer.
    vkCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateCommandBuffers(vkDevice, &allocInfo, vkCommandBuffers.data()) != VK_SUCCESS) {
        LogError(LogType::Vulkan, "Failed to allocate command buffers.");
        throw std::runtime_error("VULKAN_COMMAND_BUFFER_ERROR");
    }
}

void RendererShadows::CreateSyncObjects()
{
    const VkDevice vkDevice = renderer->GetVkDevice();
    
    // Set the semaphore and fence creation information.
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Create the semaphore and fence.
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &vkImageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &vkRenderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence    (vkDevice, &fenceInfo,     nullptr, &vkInFlightFence)           != VK_SUCCESS)
        {
            LogError(LogType::Vulkan, "Failed to create semaphores and fence.");
            throw std::runtime_error("VULKAN_SYNC_OBJECTS_ERROR");
        }
    }
}
*/
#pragma endregion

#pragma region Recreation & Destruction
void RendererShadows::RecreateShadowImage()
{
    WaitUntilIdle();
    DestroyShadowImage();
    CreateShadowImage();
}

void RendererShadows::DestroyShadowImage() const
{
    const VkDevice vkDevice = renderer->GetVkDevice();
    vkDestroyFramebuffer(vkDevice, vkShadowFramebuffer, nullptr);
    vkDestroyImageView  (vkDevice, vkShadowImageView,   nullptr);
    vkDestroyImage      (vkDevice, vkShadowImage,       nullptr);
    vkFreeMemory        (vkDevice, vkShadowImageMemory, nullptr);
}
#pragma endregion 

#pragma region Rendering
void RendererShadows::NewFrame()
{
    /*
    const VkDevice vkDevice = renderer->GetVkDevice();
    // Wait for the previous frame to finish.
    vkWaitForFences(vkDevice, 1, &vkInFlightFence, VK_TRUE, UINT64_MAX);

    // Acquire an image from the swap chain.
    const VkResult result = vkAcquireNextImageKHR(vkDevice, vkSwapChain, UINT64_MAX, vkImageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &vkSwapChainImageIndex);

    // Recreate the swap chain if it is out of date.
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateDepthImage();
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LogError(LogType::Vulkan, "Failed to acquire swap chain image.");
        throw std::runtime_error("VULKAN_IMAGE_ACQUISITION_ERROR");
    }

    // Only reset fences if we are going to be submitting work.
    vkResetFences(vkDevice, 1, &vkInFlightFence);
    */
}

void RendererShadows::BeginRenderPass() const
{
    const VkCommandBuffer vkCommandBuffer = renderer->GetCurVkCommandBuffer();

    // Define the clear color.
    VkClearValue clearValue{};
    clearValue.depthStencil = { 1.0f, 0 };

    // Begin the render pass.
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = vkRenderPass;
    renderPassInfo.framebuffer       = vkShadowFramebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = { vkShadowImageWidth, vkShadowImageHeight };
    renderPassInfo.clearValueCount   = 1;
    renderPassInfo.pClearValues      = &clearValue;
    vkCmdBeginRenderPass(vkCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind the graphics pipeline.
    vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);
}

void RendererShadows::UpdateViewportScissor(const Resources::LightType lightType) const
{
    const VkCommandBuffer vkCommandBuffer = renderer->GetCurVkCommandBuffer();
    
    if (lightType != Resources::LightType::Point)
    {
        // Set the viewport.
        VkViewport viewport{};
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = (float)vkShadowImageWidth;
        viewport.height   = (float)vkShadowImageHeight;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(vkCommandBuffer, 0, 1, &viewport);

        // Set the scissor.
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = { vkShadowImageWidth, vkShadowImageHeight };
        vkCmdSetScissor(vkCommandBuffer, 0, 1, &scissor);
    }
    else
    {
        const uint32_t faceWidth  = vkShadowImageWidth  / 2;
        const uint32_t faceHeight = vkShadowImageHeight / 2;

        VkViewport viewport{};
        switch (renderIdx)
        {
        case 0: // Green Face
            viewport.x = 0;
            viewport.y = 0;
            break;
        case 1: // Yellow Face
            viewport.x = (float)faceWidth;
            viewport.y = 0;
            break;
        case 2: // Blue Face
            viewport.x = 0;
            viewport.y = (float)faceHeight;
            break;
        case 3: // Red Face
            viewport.x = (float)faceWidth;
            viewport.y = (float)faceHeight;
            break;
        default:
            return;
        }
        
        // Set the viewport.
        viewport.width  = (float)faceWidth;
        viewport.height = (float)faceHeight;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(vkCommandBuffer, 0, 1, &viewport);

        // Set the scissor.
        VkRect2D scissor{};
        scissor.offset = { static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y) };
        scissor.extent = { faceWidth, faceHeight };
        vkCmdSetScissor(vkCommandBuffer, 0, 1, &scissor);
    }
}

void RendererShadows::EndRenderPass() const
{
    // End the render pass.
    vkCmdEndRenderPass(renderer->GetCurVkCommandBuffer());
}
#pragma endregion
