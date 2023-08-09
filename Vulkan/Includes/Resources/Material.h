#pragma once
#include "Maths/Color.h"
#include <string>
#include <vector>

#include "Core/VulkanUtils.h"

typedef struct VkDescriptorSetLayout_T* VkDescriptorSetLayout;
typedef struct VkDescriptorPool_T*      VkDescriptorPool;
typedef struct VkDescriptorSet_T*       VkDescriptorSet;

namespace Resources
{
    class Texture;

    // Enumerates all unique material texture/map types.
    namespace MaterialTextureType
    {
        enum
        {
            Albedo,    // Texture used for the overall color of the object.
            Emissive,  // Texture used for the color of light emitted by the object.
            Shininess, // Texture map used to modify the object's shininess.
            Alpha,     // Texture map used to modify the object's transparency.
            Normal,    // Texture map used to modify the object's normals.
        };
    };
    
    class Material
    {
    public:
        std::string name;
        
        Maths::RGB albedo   = 1;  // The overall color of the object.
        Maths::RGB emissive = 0;  // The color of light emitted by the object.
        float shininess     = 32; // The intensity of highlights on the object.
        float alpha         = 1;  // Defines how see-through the object is.

        static constexpr size_t textureTypesCount = 5;
        Texture* textures[textureTypesCount] = { nullptr, nullptr, nullptr, nullptr, nullptr };
        
    private:
        inline static VkDescriptorSetLayout vkDescriptorSetLayout = nullptr;
        inline static VkDescriptorPool      vkDescriptorPool      = nullptr;
        VkDescriptorSet vkDescriptorSets[VulkanUtils::MAX_FRAMES_IN_FLIGHT];

    public:
        Material(const Maths::RGB& _albedo = 1, const Maths::RGB& _emissive = 0, const float& _shininess = 32, const float& _alpha = 1,
                 Texture* albedoTexture = nullptr, Texture* emissiveTexture = nullptr, Texture* shininessMap = nullptr, Texture* alphaMap = nullptr, Texture* normalMap = nullptr);

        void SetParams(const Maths::RGB& _albedo, const Maths::RGB& _emissive, const float& _shininess, const float& _alpha);
        bool IsLoadingFinalized() const;
        void FinalizeLoading();
        
		static void CreateDescriptorLayoutAndPool (const VkDevice& vkDevice);
		static void DestroyDescriptorLayoutAndPool(const VkDevice& vkDevice);
        static VkDescriptorSetLayout GetVkDescriptorSetLayout() { return vkDescriptorSetLayout; }
        static VkDescriptorPool      GetVkDescriptorPool     () { return vkDescriptorPool;      }
               VkDescriptorSet       GetVkDescriptorSet(const uint32_t& currentFrame) const;
    };
}
