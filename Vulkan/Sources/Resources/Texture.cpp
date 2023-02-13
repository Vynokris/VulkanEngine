#include "Resources/Texture.h"
#include "Core/Application.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <vulkan/vulkan.h>
#include <iostream>
using namespace Core;
using namespace Resources;

Texture::Texture(const char* name, const Renderer* renderer)
{
    Load(name, renderer);
}

Texture::Texture(const int& width, const int& height, const Renderer* renderer)
{
    Load(width, height, renderer);
}

void Texture::Load(const char* name, const Renderer* renderer)
{
    filename = name;
    
    // Load the texture data.
    stbi_uc* pixels = stbi_load(name, &width, &height, &channels, STBI_rgb_alpha);
    const VkDeviceSize imageSize = width * height * 4;
    if (!pixels) {
        std::cout << "WARNING (STBI): Unable to load texture " << name << std::endl;
    }

    // Get the renderer and Vulkan device.
    if (renderer == nullptr) renderer       = Application::Get()->GetRenderer();
    const VkDevice           device         = renderer->GetVkDevice();
    const VkPhysicalDevice   physicalDevice = renderer->GetVkPhysicalDevice();
    const VkCommandPool      commandPool    = renderer->GetVkCommandPool();
    const VkQueue            graphicsQueue  = renderer->GetVkGraphicsQueue();

    // Create a transfer buffer to send the pixels to the GPU.
    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VulkanUtils::CreateBuffer(device, physicalDevice, imageSize,
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              stagingBuffer, stagingBufferMemory);

    // Copy the pixels to the transfer buffer.
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, (size_t)imageSize);
    vkUnmapMemory(device, stagingBufferMemory);
    stbi_image_free(pixels);

    // Create the Vulkan image.
    VulkanUtils::CreateImage(device, physicalDevice, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             vkImage, vkImageMemory);

    // Copy the transfer buffer to the vulkan image.
    VulkanUtils::TransitionImageLayout(device, commandPool, graphicsQueue, vkImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VulkanUtils::CopyBufferToImage    (device, commandPool, graphicsQueue, stagingBuffer, vkImage, (uint32_t)width, (uint32_t)height);
    VulkanUtils::TransitionImageLayout(device, commandPool, graphicsQueue, vkImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Cleanup allocated structures.
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory   (device, stagingBufferMemory, nullptr);

    // Create the texture image view.
    VulkanUtils::CreateImageView(device, vkImage, VK_FORMAT_R8G8B8A8_SRGB, vkImageView);
}

void Texture::Load(const int& width, const int& height, const Renderer* renderer)
{
    std::cout << "WARNING (Texture): Texture creation from width and height isn't implemented." << std::endl;
}

Texture::~Texture()
{
    if (vkImage       != nullptr) vkDestroyImage    (Application::Get()->GetRenderer()->GetVkDevice(), vkImage,       nullptr);
    if (vkImageMemory != nullptr) vkFreeMemory      (Application::Get()->GetRenderer()->GetVkDevice(), vkImageMemory, nullptr);
    if (vkImageView   != nullptr) vkDestroyImageView(Application::Get()->GetRenderer()->GetVkDevice(), vkImageView,   nullptr);
    vkImage       = nullptr;
    vkImageMemory = nullptr;
    vkImageView   = nullptr;
}
