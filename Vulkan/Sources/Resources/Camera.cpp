#include "Resources/Camera.h"
using namespace Resources;
using namespace Maths;

Camera::Camera(const CameraParams& parameters)
{
    transform.SetIsCamera(true);
    ChangeParams(parameters);
}

void Camera::ChangeParams(const CameraParams& parameters)
{
    params = CameraParams(parameters);
    params.aspect = (float)params.width / (float)params.height;

    // Update the projection matrix.
    const float yScale = 1 / tan(degToRad(params.fov) * 0.5f);
    const float xScale = yScale / params.aspect;
    projMat = Mat4(
        xScale, 0, 0, 0,
        0, yScale, 0, 0,
        0, 0, params.far / (params.near - params.far), -1,
        0, 0, params.far * params.near / (params.near - params.far), 0
    );
}
