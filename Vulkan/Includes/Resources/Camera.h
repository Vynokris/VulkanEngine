#pragma once
#include "Maths/Transform.h"

namespace Resources
{
	struct CameraParams
	{
		int   width = 0, height = 0;
		float near = 0, far = 1, fov = 80, aspect = 1;
	};
	
	class Camera
	{
	private:
		Maths::Mat4  projMat;
		CameraParams params;

	public:
		Maths::Transform transform;

		Camera(const CameraParams& parameters);
        void ChangeParams(const CameraParams& parameters);
		
		CameraParams GetParams () const { return params;  }
		Maths::Mat4  GetProjMat() const { return projMat; }
		Maths::Mat4  GetViewMat() const { return transform.GetLocalMat(); }
	};
}
