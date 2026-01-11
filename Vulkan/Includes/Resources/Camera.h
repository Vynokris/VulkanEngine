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
		Camera(const Camera&)            = delete;
		Camera(Camera&&)                 = delete;
		Camera& operator=(const Camera&) = delete;
		Camera& operator=(Camera&&)      = delete;
        void SetParams(const CameraParams& parameters);
		
		const CameraParams& GetParams () const { return params;  }
		const Maths::Mat4&  GetProjMat() const { return projMat; }
		const Maths::Mat4&  GetViewMat() const { return transform.GetViewMat(); }
		Maths::Mat4 ComputeViewProjMat() const { return GetViewMat() * GetProjMat(); }
	};
}
