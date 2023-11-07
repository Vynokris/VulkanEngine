#pragma once
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
        static constexpr size_t COUNT = 5; // The number of different texture types that are stored in a material's textures array.
        
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
        std::string name; // The material's name.
        
        Maths::RGB albedo   = 1;  // The overall color of the object.
        Maths::RGB emissive = 0;  // The color of light emitted by the object.
        float shininess     = 32; // The intensity of highlights on the object.
        float alpha         = 1;  // Defines how see-through the object is.

        Texture* textures[MaterialTextureType::COUNT] = { nullptr, nullptr, nullptr, nullptr, nullptr }; // Array of all different textures used by this material.
        
    private:
        inline static VkDescriptorSetLayout vkDescriptorSetLayout = nullptr;
        inline static VkDescriptorPool      vkDescriptorPool      = nullptr;
        VkDescriptorSet vkDescriptorSet    = nullptr;
        VkBuffer        vkDataBuffer       = nullptr;
        VkDeviceMemory  vkDataBufferMemory = nullptr;

    public:
        Material(const Maths::RGB& _albedo = 1, const Maths::RGB& _emissive = 0, const float& _shininess = 32, const float& _alpha = 1,
                 Texture* albedoTexture = nullptr, Texture* emissiveTexture = nullptr, Texture* shininessMap = nullptr, Texture* alphaMap = nullptr, Texture* normalMap = nullptr);
        Material(const Material&)            = delete;
        Material(Material&&)                 = delete;
        Material& operator=(const Material&) = delete;
        Material& operator=(Material&&)      noexcept;
        ~Material();
        
        void SetParams(const Maths::RGB& _albedo, const Maths::RGB& _emissive, const float& _shininess, const float& _alpha);
        bool IsLoadingFinalized() const { return vkDescriptorSet && vkDataBuffer && vkDataBufferMemory; }
        void FinalizeLoading();
        
		static void CreateDescriptorLayoutAndPool (const VkDevice& vkDevice);
		static void DestroyDescriptorLayoutAndPool(const VkDevice& vkDevice);
        static VkDescriptorSetLayout GetVkDescriptorSetLayout()       { return vkDescriptorSetLayout; }
        static VkDescriptorPool      GetVkDescriptorPool     ()       { return vkDescriptorPool;      }
               VkDescriptorSet       GetVkDescriptorSet      () const { return vkDescriptorSet;       }
    };
}
