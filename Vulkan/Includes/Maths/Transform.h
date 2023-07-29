#pragma once

#include "Vector3.h"
#include "Quaternion.h"
#include "Matrix.h"

namespace Maths
{
	// - Transform: position, rotation and scale in 3D space, with local, parent and world transformation matrices - //
	// * By default, this is used to transform objects. To transform cameras, set isCamera to true. * //
    class Transform
    {
	private:
		Vector3    pos;
		Quaternion rot;
    	Vector3    scale;
		bool       isCamera;
		Mat4       localMat, parentMat, worldMat, viewMat;
    	Vector3    worldPos, eulerRot;

	public:
    	// -- Constructors -- //
    	
		Transform(const bool& _isCamera = false); // Transform with default values and optional parameter to set transform type (camera or object).
		Transform(const Vector3& position, const Quaternion& rotation, const Vector3& _scale, const bool& _isCamera = false); // Transform from position, rotation and scale and optional parameter to set transform type (camera or object).

        // -- Position -- //
		Vector3 GetPosition() const;                  // Returns the transform's position.
    	Vector3 GetWorldPosition() const;             // Returns the transform's position taking the parent transform into account.
		void    SetPosition(const Vector3& position); // Modifies the transform's position.
		void    Move       (const Vector3& movement); // Adds the given vector to the transform's position.

		// -- Direction vectors -- //
		Vector3 Forward() const;                    // Returns the transform's forward vector.
		Vector3 Up     () const;                    // Returns the transform's up vector.
		Vector3 Right  () const;                    // Returns the transform's right vector.
		void    SetForward(const Vector3& forward); // Modifies the transform's rotation to point in the given direction.

		// -- Rotation -- //
		Quaternion GetRotation() const;                     // Returns the transform's quaternion rotation.
		Vector3    GetEulerRot() const;                     // Returns the transform's euler angles rotation.
		void       SetRotation(const Quaternion& rotation); // Modifies the transform's quaternion rotation.
    	void       SetEulerRot(const Vector3& euler);       // Modifies the transform's euler angles rotation.
		void       Rotate     (const Quaternion& rotation); // Rotates the transform by the given quaternion.
    	void       RotateEuler(const Vector3& euler);       // Rotates the transform by the given euler angles.

		// -- Scale -- //
		Vector3 GetScale       () const;         // Returns the transform's scale.
		Vector3 GetUniformScale() const;		 // Returns the maximum value between the x, y and z elements of the transform's scale.
		void    SetScale(const Vector3& _scale); // Sets the transform's scale to the given one.
    	void    Scale   (const Vector3& _scale); // Scales the transform by the given value.

    	// -- Is Camera -- //
    	bool IsCamera() const;                   // Returns true if the transform is a camera transform, false if it is an object transform.
    	void SetIsCamera(const bool& _isCamera); // Modifies the transform's type (object transform or camera transform).

    	// -- Setting multiple values at a time -- //
    	void SetPosRot(const Vector3& position, const Quaternion& rotation);                        // Modifies the transform's position and rotation.
    	void SetValues(const Vector3& position, const Quaternion& rotation, const Vector3& _scale); // Modifies all of the transform's values.

		// -- Matrices -- //
		Mat4 GetLocalMat () const { return localMat;  } // Returns the transform's local matrix.
    	Mat4 GetViewMat  () const { return isCamera ? viewMat : Mat4(); } // Returns the transform's view matrix (identity if not camera).
		Mat4 GetParentMat() const { return parentMat; } // Returns the transform's parent matrix.
    	Mat4 GetWorldMat () const { return worldMat;  } // Returns the transform's world matrix (local * parent).
		void SetParentMat(const Mat4& mat);             // Modifies the transform's parent matrix.

    	// -- Interpolation -- //
    	static Transform Lerp(const Transform& start, const Transform& dest, const float& val, const bool& useSlerp = true); // Linearly interpolates between start and dest.

	private:
		void UpdateMatrices();
    };

	// - RawTransform: position, rotation and scale in 3D space with no additional functionalities - //
	struct RawTransform
	{
		Vector3    position;
		Quaternion rotation;
		Vector3    scale;
	};
}