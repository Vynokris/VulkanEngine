#pragma once
#include "Maths/Color.h"

namespace Resources
{
    class Texture;
    class Material
    {
    public:
        std::string name;
        
        Maths::RGB albedo   = 1;  // The overall color of the object.
        Maths::RGB emissive = 0;  // The color of light emitted by the object.
        float shininess     = 32; // The intensity of highlights on the object.
        float alpha         = 1;  // Defines how see-through the object is.

        Texture* albedoTexture   = nullptr; // Texture used for the overall color of the object.
        Texture* emissiveTexture = nullptr; // Texture used for the color of light emitted by the object.
        Texture* shininessMap    = nullptr; // Texture used to modify the object's shininess.
        Texture* alphaMap        = nullptr; // Texture used to modify the object's transparency.

    public:
        Material(const Maths::RGB& _albedo = 1, const Maths::RGB& _emissive = 0, const float& _shininess = 32, const float& _alpha = 1,
                 Texture* _albedoTexture = nullptr, Texture* _emissiveTexture = nullptr, Texture* _shininessMap = nullptr, Texture* _alphaMap = nullptr)
            : albedo(_albedo), emissive(_emissive), shininess(_shininess), alpha(_alpha),
              albedoTexture(_albedoTexture), emissiveTexture(_emissiveTexture), shininessMap(_shininessMap), alphaMap(_alphaMap) {}

        
        void SetParams(const Maths::RGB& _albedo, const Maths::RGB& _emissive, const float& _shininess, const float& _alpha)
        {
            albedo    = _albedo;
            emissive  = _emissive;
            shininess = _shininess;
            alpha     = _alpha;
        }
    };
}
