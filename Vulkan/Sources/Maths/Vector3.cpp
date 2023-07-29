#include "Maths/Maths.h"
#include <cmath>
using namespace Maths;


// Constructors.
Vector3::Vector3()                                                  : x(0),   y(0),   z(0)                           {}; // Null vector.
Vector3::Vector3(const float& all)                                  : x(all), y(all), z(all)                         {}; // Vector with equal coordinates.
Vector3::Vector3(const float& _x, const float& _y, const float& _z) : x(_x),  y(_y),  z(_z)                          {}; // Vector with 3 coordinates.
Vector3::Vector3(const Vector3& p1, const Vector3& p2)              : x(p2.x - p1.x), y(p2.y - p1.y), z(p2.z - p1.z) {}; // Vector from 2 points.
Vector3::Vector3(const Vector3& angles, const float& length)        : x(0),   y(0),   z(0)                               // Vector from angles and length.
{
    *this = (Vector4(0, 0, 1, 1) * Mat4::FromEuler(-angles)).ToVector3(true) * length;
}
Vector3 Vector3::FromSphericalCoords(const Vector3& spherical)
{
    const float longitude = spherical.x;
    const float latitude  = spherical.y;
    const float length    = spherical.z;
    const float sinLongitude = sin(longitude);
    return Vector3(
        sinLongitude * cos(latitude),
        cos(longitude),
        sinLongitude * sin(latitude)
    ) * length;
}

// ---------- VECTOR3 OPERATORS ---------- //

// Vector3 negation.
Vector3 Vector3::operator-() const { return { -x, -y, -z }; }

// Vector3 dot product.
float Vector3::Dot(const Vector3& v) const { return x*v.x + y*v.y + z*v.z; }

// Vector3 cross product.
Vector3 Vector3::Cross(const Vector3& v) const { return Vector3(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x); }

// ------------ VECTOR3 METHODS ----------- //

// Length.
float   Vector3::GetLength  () const { return sqrt(GetLengthSq()); }
float   Vector3::GetLengthSq() const { return sqpow(x) + sqpow(y) + sqpow(z); }
void    Vector3::SetLength        (const float& length)       { *this = GetModifiedLength(length); }
Vector3 Vector3::GetModifiedLength(const float& length) const { return  GetNormalized() * length;  }

// Normalization.
void Vector3::Normalize()
{
    const float length = GetLength();
    if (length < 1e-4f) { x=y=z=0; return; }
    x /= length;
    y /= length;
    z /= length;
}
Vector3 Vector3::GetNormalized() const
{
    const float length = GetLength();
    if (length < 1e-4f) return Vector3();
    return Vector3(x / length, y / length, z / length);
}

// Negation.
void    Vector3::Negate    ()       { x *= -1; y *= -1; z *= -1; }
Vector3 Vector3::GetNegated() const { return Vector3(x*-1, y*-1, z*-1); }

// Copy signs.
void    Vector3::CopySign     (const Vector3& source)       { *this = GetCopiedSign(source); }
Vector3 Vector3::GetCopiedSign(const Vector3& source) const { return Vector3(std::copysign(x, source.x), std::copysign(y, source.y), std::copysign(z, source.z)); }

// Interprets the vector as a point and returns the distance to another point.
float Vector3::GetDistanceFromPoint(const Vector3& p) const { return Vector3(*this, p).GetLength(); }

// Interpolation.
Vector3 Vector3::Lerp(const Vector3& start, const Vector3& dest, const float& val)
{
    return Vector3(lerp(start.x, dest.x, val),
                   lerp(start.y, dest.y, val),
                   lerp(start.z, dest.z, val));
}

// Returns this point in spherical coords (x: length, y: longitude, z: latitude).
Vector3 Vector3::ToSphericalCoords() const
{
    const float length    = GetLength();
    const float length2D  = Vector2(x, z).GetLength();
    const float longitude = (length   < 1e-4f ? 0 : acos(y / length));
    const float latitude  = (length2D < 1e-4f ? 0 : signof(z) * acos(x / length2D));
    return { longitude, latitude, length };
}


Vector4 Vector3::ToVector4(const float& w) const { return Vector4(x, y, z, w); } // Creates a 4D vector from this vector.
Vector2 Vector3::ToVector2()               const { return Vector2(x, y);       } // Creates a 2D vector by dropping this vector's z component.

// Returns the vector's contents as a string.
std::string Vector3::ToString(const int& precision) const
{
    std::ostringstream string;
    string.precision(precision);
    string << std::fixed << x << ", " << std::fixed << y << ", " << std::fixed << z;
    return string.str();
}
