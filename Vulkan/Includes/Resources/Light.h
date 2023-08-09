#pragma once
#include "Maths/Color.h"
#include "Maths/MathConstants.h"
#include "Maths/Vector3.h"

namespace Resources
{
    enum class LightType
    {
        Directional,
        Point,
        Spot,
    };
    
    class Light
    {
    public:
        LightType      type;                           // The type of emitter this light is part of.
        Maths::RGB     albedo;                         // The color of light emitted.
        Maths::Vector3 position;                       // The light's position in 3D space.
        Maths::Vector3 direction;                      // The light's direction in 3D space.
        float brightness = 1;                          // Defines how bright the light is.
        float constant = 1, linear = 0, quadratic = 0; // The light's constant, linear and quadratic attenuations.
        float outerCutoff = 0, innerCutoff = 0;        // The light's inner and outer cutoffs.

        static Light Directional(const Maths::RGB& _albedo = {1,1,1}, const Maths::Vector3& _direction = {0,0,1}, const float& _brightness = 1);
        static Light Point      (const Maths::RGB& _albedo = {1,1,1}, const Maths::Vector3& _position  = {0,0,0}, const float& _brightness = 1, const float& _constant = 1, const float& _linear = 0, const float& _quadratic = 0);
        static Light Spot       (const Maths::RGB& _albedo = {1,1,1}, const Maths::Vector3& _position  = {0,0,0}, const Maths::Vector3& _direction = {0,0,1}, const float& _brightness = 1, const float& _constant = 1, const float& _linear = 0, const float& _quadratic = 0, const float& _outerCutoff = PIDIV4, const float& _innerCutoff = PIDIV4 * 0.5f);
    };
}