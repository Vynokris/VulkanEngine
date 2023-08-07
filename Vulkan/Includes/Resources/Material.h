#pragma once
#include "Maths/Color.h"

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

    public:
        Material(const Maths::RGB& _albedo = 1, const Maths::RGB& _emissive = 0, const float& _shininess = 32, const float& _alpha = 1,
                 Texture* albedoTexture = nullptr, Texture* emissiveTexture = nullptr,
                 Texture* shininessMap = nullptr, Texture* alphaMap = nullptr, Texture* normalMap = nullptr)
            : albedo(_albedo), emissive(_emissive), shininess(_shininess), alpha(_alpha)
        {
            textures[MaterialTextureType::Albedo   ] = albedoTexture;
            textures[MaterialTextureType::Emissive ] = emissiveTexture;
            textures[MaterialTextureType::Shininess] = shininessMap;
            textures[MaterialTextureType::Alpha    ] = alphaMap;
            textures[MaterialTextureType::Normal   ] = normalMap;
        }

        void SetParams(const Maths::RGB& _albedo, const Maths::RGB& _emissive, const float& _shininess, const float& _alpha)
        {
            albedo    = _albedo;
            emissive  = _emissive;
            shininess = _shininess;
            alpha     = _alpha;
        }
    };
}
