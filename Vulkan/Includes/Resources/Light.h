#pragma once
#include "Maths/Color.h"
#include "Maths/Vector3.h"

namespace Resources
{
    class Light
    {
    public:
        Maths::RGB ambient, diffuse, specular;
        Maths::Vector3 position, direction;
        float constant = 1, linear = 0.025f, quadratic = 0.01f;
        float outerCone = PI/6, innerCone = PI/12;
    };
}
