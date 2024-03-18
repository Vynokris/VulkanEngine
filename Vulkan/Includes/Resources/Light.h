#pragma once
#include "Maths/Color.h"
#include "Maths/MathConstants.h"
#include "Maths/Vector3.h"
#include <vector>

namespace Core { template<typename T> struct GpuArray; }
namespace Resources
{
    enum class LightType
    {
        Unassigned,
        Directional,
        Point,
        Spot,
    };
    
    class Light
    {
    public:
        LightType type;                                // The type of emitter this light is part of.
        alignas(16) Maths::RGB     albedo;             // The color of light emitted.
        alignas(16) Maths::Vector3 position;           // The light's position in 3D space.
        alignas(16) Maths::Vector3 direction;          // The light's direction in 3D space.
        float brightness = 1, radius = 1, falloff = 0; // The light's brightness, effective radius and distance falloff.
        float outerCutoff = 0, innerCutoff = 0;        // The light's inner and outer cutoffs.

        static Light Directional(const Maths::RGB& _albedo = {1,1,1}, const Maths::Vector3& _direction = {0,0,1}, const float& _brightness = 1);
        static Light Point      (const Maths::RGB& _albedo = {1,1,1}, const Maths::Vector3& _position  = {0,0,0}, const float& _brightness = 1, const float& _radius = 5, const float& _falloff = 4);
        static Light Spot       (const Maths::RGB& _albedo = {1,1,1}, const Maths::Vector3& _position  = {0,0,0}, const Maths::Vector3& _direction = {0,0,1}, const float& _brightness = 1, const float& _radius = 5, const float& _falloff = 4, const float& _outerCutoff = PIDIV4, const float& _innerCutoff = PIDIV4 * 0.5f);

        static void UpdateBufferData(const std::vector<Light>& lights, const Core::GpuArray<Light>* lightArray = nullptr);
    };
}