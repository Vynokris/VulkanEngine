#include "Resources/Camera.h"
using namespace Resources;
using namespace Maths;

Camera::Camera(const CameraParams& parameters)
{
    ChangeParams(parameters);
}

void Camera::ChangeParams(const CameraParams& parameters)
{
    params = CameraParams(parameters);
    params.aspect = (float)params.width / params.height;

    // Update the projection matrix.
    /*
    float yScale = 1 / tan(degToRad(params.fov / 2));
    float xScale = yScale / params.aspect;
    projMat = Mat4(
        -xScale, 0, 0, 0,
        0, yScale, 0, 0,
        0, 0, params.far / (params.far - params.near), 1,
        0, 0, -params.far * params.near / (params.far - params.near), 0
    );
    */
    const float yScale = 1 / tan(degToRad(params.fov) * 0.5f);
    const float xScale = 1 / (yScale * params.aspect);
    projMat = Mat4(
        xScale, 0, 0, 0,
        0, yScale, 0, 0,
        0, 0, -(params.far + params.near) / (params.far - params.near), -1,
        0, 0, -(2 * params.far * params.near) / (params.far - params.near), 1
    );
}
