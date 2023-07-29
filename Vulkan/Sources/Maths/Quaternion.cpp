#include "Maths/Maths.h"
using namespace Maths;


// ----- Constructors ----- //

Quaternion::Quaternion()                                                               : w(1),   x(0),   y(0),   z(0)   {}
Quaternion::Quaternion(const float& all)                                               : w(all), x(all), y(all), z(all) {}
Quaternion::Quaternion(const float& w, const float& x, const float& y, const float& z) : w(w),   x(x),   y(y),   z(z)   {}
Quaternion::Quaternion(const Vector3& eulerAngles)                                     : w(1),   x(0),   y(0),   z(0)
{
    const float cX = cos(eulerAngles.x * 0.5f), sX = sin(eulerAngles.x * 0.5f);
    const float cY = cos(eulerAngles.y * 0.5f), sY = sin(eulerAngles.y * 0.5f);
    const float cZ = cos(eulerAngles.z * 0.5f), sZ = sin(eulerAngles.z * 0.5f);
    
    w = cX*cY*cZ - sX*sY*sZ;
    x = sX*cY*cZ + cX*sY*sZ;
    y = cX*sY*cZ - sX*cY*sZ;
    z = cX*cY*sZ + sX*sY*cZ;
}
Quaternion::Quaternion(const AngleAxis& angleAxis) { *this = angleAxis.ToQuaternion(); }
Quaternion::Quaternion(const Mat4&      matrix   ) { *this = matrix   .ToQuaternion(); }


// ----- Static constructors  ----- //

Quaternion Quaternion::FromPitch    (const float&     angle    ) { return Quaternion(cos(angle/2.f), sin(angle/2.f), 0, 0); }
Quaternion Quaternion::FromRoll     (const float&     angle    ) { return Quaternion(cos(angle/2.f), 0, sin(angle/2.f), 0); }
Quaternion Quaternion::FromYaw      (const float&     angle    ) { return Quaternion(cos(angle/2.f), 0, 0, sin(angle/2.f)); }
Quaternion Quaternion::FromEuler    (const Vector3&   angles   ) { return Quaternion(angles); }
Quaternion Quaternion::FromAngleAxis(const AngleAxis& angleAxis) { return angleAxis.ToQuaternion(); }
Quaternion Quaternion::FromMatrix   (const Mat4&      matrix   ) { return matrix   .ToQuaternion(); }

// ----- Operators  ----- //

Quaternion Quaternion::operator-() const
{
    return { -w, -x, -y, -z };
}

float Quaternion::Dot(const Quaternion& q) const
{
    return w*q.w + x*q.x + y*q.y + z*q.z;
}


// ----- Methods ----- //

// Absolute value.
float Quaternion::GetModulus() const
{
    return sqrt(sqpow(w) + sqpow(x) + sqpow(y) + sqpow(z));
}

// Normalization.
void Quaternion::Normalize()
{
    *this = GetNormalized();
}
Quaternion Quaternion::GetNormalized() const
{
    const float modulus = GetModulus();
    return Quaternion(w/modulus, x/modulus, y/modulus, z/modulus);
}

// Conjugation.
void       Quaternion::Conjugate   ()       { x *= -1; y *= -1; z *= -1; }
Quaternion Quaternion::GetConjugate() const { return { w, -x, -y, -z }; }

// Inversion.
void Quaternion::Inverse()
{
    *this = GetInverse();
}
Quaternion Quaternion::GetInverse() const
{
    const float sqAbs = sqpow(w) + sqpow(x) + sqpow(y) + sqpow(z);
    return Quaternion(w / sqAbs, -x / sqAbs, -y / sqAbs, -z / sqAbs);
}

// Rotation.
Quaternion Quaternion::RotateQuat(const Quaternion& q) const
{
    return *this * q;
}
Vector3 Quaternion::RotateVec(const Vector3& v) const
{
    const Quaternion vecQuat = { 0, v.x, v.y, v.z };
    const Quaternion rotated = *this * vecQuat * GetConjugate();
    return { rotated.x, rotated.y, rotated.z };
}

// Interpolation.
Quaternion Quaternion::Lerp(const Quaternion& start, const Quaternion& dest, const float& val)
{
    return Quaternion(lerp(start.w, dest.w, val),
                      lerp(start.x, dest.x, val),
                      lerp(start.y, dest.y, val),
                      lerp(start.z, dest.z, val));
}

Quaternion Quaternion::NLerp(const Quaternion& start, const Quaternion& dest, const float& val)
{
    return Lerp(start, dest, val).GetNormalized();
}

Quaternion Quaternion::SLerp(const Quaternion& start, const Quaternion& dest, const float& val, const bool& useShortestPath)
{
    const float cosAngle    = start.Dot(dest);
    const float cosAngleAbs = abs(cosAngle);
    
    float coeff1, coeff2;
    if (1-cosAngleAbs < 0.01f)
    {
        // Linear interpolation for close orientations.
        coeff1 = 1-val;
        coeff2 = val;
    }
    else
    {
        // Spherical interpolation.
        const float angle    = acos(cosAngleAbs);
        const float sinAngle = sin(angle);
        coeff1 = sin(angle * (1-val)) / sinAngle;
        coeff2 = sin(angle * val)     / sinAngle;
    }

    // Use the shortest path.
    if (useShortestPath && cosAngle < 0)  coeff1 *= -1;

    // Return the interpolated quaternion.
    return Quaternion (coeff1 * start.w + coeff2 * dest.w,
                       coeff1 * start.x + coeff2 * dest.x,
                       coeff1 * start.y + coeff2 * dest.y,
                       coeff1 * start.z + coeff2 * dest.z).GetNormalized();
}


// ----- Conversions ----- //

// Angle-axis.
AngleAxis Quaternion::ToAngleAxis() const
{
    const float angle = 2 * acos(w);
    const float temp  = sqrt(1 - sqpow(w));
    if (temp < 1e-3f)
        return { angle, { 0, 0, 1 } };
    return { angle, Vector3(x, y, z) / temp };
}

// Rotation matrix.
Mat4 Quaternion::ToMatrix() const
{
    const float twoX = x + x;
    const float twoY = y + y;
    const float twoZ = z + z;
    const float xx = x * twoX;
    const float xy = x * twoY;
    const float xz = x * twoZ;
    const float yy = y * twoY;
    const float yz = y * twoZ;
    const float zz = z * twoZ;
    const float wx = w * twoX;
    const float wy = w * twoY;
    const float wz = w * twoZ;
    
    return Mat4(1-(yy+zz), xy+wz,     xz-wy,     0,
               xy-wz,      1-(xx+zz), yz+wx,     0,
               xz+wy,      yz-wx,     1-(xx+yy), 0,
               0,          0,         0,         1);
}

// Euler angles.
Vector3 Quaternion::ToEuler() const
{
    Vector3 euler;
    const Mat4 mat = ToMatrix();
    euler.y = asin(clamp(mat.m[2][0], -1, 1));
    if (0.99999f > abs(mat.m[2][0]))
    {
        euler.x = atan2(-mat.m[2][1], mat.m[2][2]);
        euler.z = atan2(-mat.m[1][0], mat.m[0][0]);
    }
    else
    {
        euler.x = atan2(mat.m[1][2], mat.m[1][1]);
        euler.z = 0;
    }
    return euler;
}

// String.
std::string Quaternion::ToString(const int& precision) const
{
    std::ostringstream string;
    string.precision(precision);
    string << std::fixed << w << ", " << std::fixed << x << ", " << std::fixed << y << ", " << std::fixed << z;
    return string.str();
}
