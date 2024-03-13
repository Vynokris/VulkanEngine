#pragma once
#include "Core/UniqueID.h"
#include "Maths/Color.h"
#include <string>

typedef struct VkDescriptorSetLayout_T* VkDescriptorSetLayout;
typedef struct VkDescriptorPool_T*      VkDescriptorPool;
typedef struct VkDescriptorSet_T*       VkDescriptorSet;
typedef struct VkBuffer_T*              VkBuffer;
typedef struct VkDeviceMemory_T*        VkDeviceMemory;
typedef struct VkDevice_T*              VkDevice;

namespace Resources
{
    class Texture;

    // Enumerates all unique material texture/map types.
    namespace MaterialTextureType
    {
        static constexpr size_t COUNT = 8; // The number of different texture types that are stored in a material's textures array.
        
        enum
        {
            Albedo,     // Texture used for the overall color of the object.
            Emissive,   // Texture used for the color of light emitted by the object.
            Metallic,   // Texture map used to modify the object's metalness.
            Roughness,  // Texture map used to modify the object's roughness.
            AOcclusion, // Texture map used to add shadows to the object.
            Alpha,      // Texture map used to modify the object's transparency.
            Normal,     // Texture map used to modify the object's normals.
            Depth,      // Texture map used to modify the object's depth through parallax mapping.
        };
    };
    
    class Material : public UniqueID
    {
    public:
        std::string  name;                   // The material's name.
        Maths::RGB   albedo          = 1;    // The overall color of the object.
        Maths::RGB   emissive        = 0;    // The color of light emitted by the object.
        float        metallic        = 0;    // The intensity of highlights on the object.
        float        roughness       = 1;    // The intensity of highlights on the object.
        float        alpha           = 1;    // Defines how see-through the object is.
        float        depthMultiplier = 0.1f; // Defines how intense the parallax depth effect should be.
        unsigned int depthLayerCount = 32;   // Defines how many layers are used in the parallax depth effect.

        Texture* textures[MaterialTextureType::COUNT] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }; // Array of all different textures used by this material.
        
    private:
        inline static VkDescriptorSetLayout vkDescriptorSetLayout = nullptr;
        inline static VkDescriptorPool      vkDescriptorPool      = nullptr;
        VkDescriptorSet vkDescriptorSet    = nullptr;
        VkBuffer        vkDataBuffer       = nullptr;
        VkDeviceMemory  vkDataBufferMemory = nullptr;

    public:
        Material(const Maths::RGB& _albedo = 1, const Maths::RGB& _emissive = 0, const float& _metallic = 1, const float& _roughness = 1, const float& _alpha = 1,
                 Texture* albedoTexture = nullptr, Texture* emissiveTexture = nullptr, Texture* metallicMap = nullptr, Texture* roughnessMap = nullptr, Texture* aoMap = nullptr, Texture* alphaMap = nullptr, Texture* normalMap = nullptr);
        Material(const Material&)            = delete;
        Material(Material&&)                 = delete;
        Material& operator=(const Material&) = delete;
        Material& operator=(Material&&)      noexcept;
        ~Material();
        
        void SetParams(const Maths::RGB& _albedo, const Maths::RGB& _emissive, const float& _metallic, const float& _roughness, const float& _alpha);
        bool IsLoadingFinalized() const { return vkDescriptorSet && vkDataBuffer && vkDataBufferMemory; }
        void FinalizeLoading();
        
		static void CreateVkData (const VkDevice& vkDevice);
		static void DestroyVkData(const VkDevice& vkDevice);
        static VkDescriptorSetLayout GetVkDescriptorSetLayout()       { return vkDescriptorSetLayout; }
        static VkDescriptorPool      GetVkDescriptorPool     ()       { return vkDescriptorPool;      }
               VkDescriptorSet       GetVkDescriptorSet      () const { return vkDescriptorSet;       }
    };
}
