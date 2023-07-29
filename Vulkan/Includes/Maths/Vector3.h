#pragma once
#include <string>

namespace Maths
{
    class Vector4;
    class Vector2;

    // - Vector3: Point in 3D space - //
    class Vector3
    {
    public:
        // -- Attributes -- //
        float x, y, z; // Vector components.

        // -- Constructors -- //
        Vector3();                                                  // Null vector.
        Vector3(const float& all);                                  // Vector with equal coordinates.
        Vector3(const float& _x, const float& _y, const float& _z); // Vector with 3 coordinates.
        Vector3(const Vector3& p1, const Vector3& p2);              // Vector from 2 points.
        Vector3(const Vector3& angles, const float& length);        // Vector from angles and length.

        // -- Static constructors -- //
        static Vector3 FromSphericalCoords(const Vector3& spherical); // Vector from spherical coordinates (x: longitude, y: latitude, z: length). Longitude and latitude are angles in radians. Latitude: 0 = top, pi = bottom.

        // -- Operators -- //
                              float&  operator[](const size_t& i)       { return *(&x + i); }
                              float   operator[](const size_t& i) const { return *(&x + i); }
        template <typename T> bool    operator==(const T& val) const;
        template <typename T> bool    operator!=(const T& val) const;
        template <typename T> Vector3 operator+ (const T& val) const;
                              Vector3 operator- (            ) const;
        template <typename T> Vector3 operator- (const T& val) const;
        template <typename T> Vector3 operator* (const T& val) const;
        template <typename T> Vector3 operator/ (const T& val) const;
        template <typename T> void    operator+=(const T& val);
        template <typename T> void    operator-=(const T& val);
        template <typename T> void    operator*=(const T& val);
        template <typename T> void    operator/=(const T& val);
                              float   Dot       (const Vector3& v) const; // Computes dot product with another vector.
                              Vector3 Cross     (const Vector3& v) const; // Computes cross product with another vector.

        // -- Length -- //
        float   GetLength  () const; // Returns the vector's length.
        float   GetLengthSq() const; // Returns the vector's length squared.
        void    SetLength        (const float& length);       // Modifies the vector's length to correspond to the given value.
        Vector3 GetModifiedLength(const float& length) const; // Returns a copy of the vector with with its length modified to the given value.

        // -- Normalization -- //
        void    Normalize();           // Normalizes the given vector so that its length is 1.
        Vector3 GetNormalized() const; // Returns a normalized copy of the vector.

        // -- Negation -- //
        void    Negate();           // Negates the vector's coordinates.
        Vector3 GetNegated() const; // Returns a negated copy of the vector.

        // -- Copy signs -- //
        void    CopySign     (const Vector3& source);       // Copies the signs from the source vector to this vector.
        Vector3 GetCopiedSign(const Vector3& source) const; // Returns a copy of this vector with the given vector's signs.

        // -- Miscellaneous -- //
        
        // Interprets the vector as a point and returns the distance to another point.
        float GetDistanceFromPoint(const Vector3& p) const;
        
        // Linearly interpolates from start to dest.
        static Vector3 Lerp(const Vector3& start, const Vector3& dest, const float& val);

        // -- Conversions -- //
        Vector3     ToSphericalCoords() const;                // Returns this point in spherical coords (x: longitude, y: latitude, z: length).
        Vector4     ToVector4(const float& w = 1) const;      // Creates a 4D vector from this vector.
        Vector2     ToVector2() const;                        // Creates a 2D vector by dropping this vector's z component.
        std::string ToString(const int& precision = 2) const; // Returns the vector's contents as a string.
    };
}

#include "Vector3.inl"
