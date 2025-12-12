#pragma once
#include "Maths/Color.h"
#include "Maths/MathConstants.h"
#include "Maths/Vector3.h"
#include <vector>

namespace Core { template<typename T> struct GpuArray; }
namespace Resources
{
    enum class LightType : int
    {
        Unassigned,
        Directional,
        Point,
        Spot,
    };
    
    class Light
    {
    public:
        Maths::RGBA albedo;                     // The color of light emitted, alpha serves as brightness.
        alignas(16) Maths::Vector3 position;    // The light's position in 3D space.
        alignas(16) Maths::Vector3 direction;   // The light's direction in 3D space.
        float radius = 1, falloff = 0;          // The light's effective radius and distance falloff.
        float outerCutoff = 0, innerCutoff = 0; // The light's inner and outer cutoffs.
        LightType type;                         // The type of emitter this light is part of.

        static Light Directional(const Maths::Vector3& _direction, const Maths::RGBA& _albedo = {1});
        static Light Point      (const Maths::Vector3& _position,  const Maths::RGBA& _albedo = {1}, const float& _radius = 5, const float& _falloff = 4);
        static Light Spot       (const Maths::Vector3& _position,  const Maths::Vector3& _direction, const Maths::RGBA& _albedo = {1}, const float& _radius = 5, const float& _falloff = 4, const float& _outerCutoff = PIDIV4, const float& _innerCutoff = PIDIV4 * 0.5f);

        static void UpdateBufferData(const std::vector<Light>& lights, const Core::GpuArray<Light>* lightsArray = nullptr);
    };
}