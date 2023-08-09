#include "Resources/Light.h"
using namespace Resources;
using namespace Maths;

Light Light::Directional(const RGB& _albedo, const Vector3& _direction, const float& _brightness)
{
    return { LightType::Directional, _albedo, {}, _direction, _brightness };
}

Light Light::Point(const RGB& _albedo, const Vector3& _position, const float& _brightness,
                   const float& _constant, const float& _linear, const float& _quadratic)
{
    return { LightType::Point, _albedo, _position, {}, _brightness, _constant, _linear, _quadratic };
}

Light Light::Spot(const RGB& _albedo, const Vector3& _position, const Vector3& _direction, const float& _brightness,
                  const float& _constant, const float& _linear, const float& _quadratic,
                  const float& _outerCutoff, const float& _innerCutoff)
{
    return { LightType::Spot, _albedo, _position, _direction, _brightness, _constant, _linear, _quadratic, _outerCutoff, _innerCutoff };
}
