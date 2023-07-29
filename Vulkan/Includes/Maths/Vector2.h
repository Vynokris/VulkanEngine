#pragma once
#include <string>

namespace Maths
{
    // - Vector2: Point in 2D space - //
    class Vector2
    {
    public:
        // -- Attributes -- //
        float x, y; // Vector components.

        // -- Constructors -- //
        Vector2();                                                           // Null vector.
        Vector2(const float& all);                                           // Vector with equal coordinates.
        Vector2(const float& _x, const float& _y);                           // Vector with 2 coordinates.
        Vector2(const Vector2& p1, const Vector2& p2);                       // Vector from 2 points.
        Vector2(const float& rad, const float& length, const bool& isAngle); // Vector from angle (bool is only used to differenciate with the (_x, _y) constructor).

        // -- Operators -- //
        template <typename T> bool    operator==(const T& val) const;
        template <typename T> bool    operator!=(const T& val) const;
        template <typename T> Vector2 operator+ (const T& val) const;
                              Vector2 operator- (            ) const;
        template <typename T> Vector2 operator- (const T& val) const;
        template <typename T> Vector2 operator* (const T& val) const;
        template <typename T> Vector2 operator/ (const T& val) const;
        template <typename T> void    operator+=(const T& val);
        template <typename T> void    operator-=(const T& val);
        template <typename T> void    operator*=(const T& val);
        template <typename T> void    operator/=(const T& val);
                              float   Dot       (const Vector2& v) const; // Computes dot product with another vector.
                              float   Cross     (const Vector2& v) const; // Computes cross product with another vector.

        // -- Length -- //
        float   GetLength  () const; // Returns the vector's length.
        float   GetLengthSq() const; // Returns the vector's length squared.
        void    SetLength        (const float& length);       // Modifies the vector's length to correspond to the given value.
        Vector2 GetModifiedLength(const float& length) const; // Returns a copy of the vector with with its length modified to the given value.

        // -- Normalization -- //
        void    Normalize();           // Normalizes the vector so that its length is 1.
        Vector2 GetNormalized() const; // Returns a normalized copy of the vector.

        // -- Negation -- //
        void    Negate();           // Negates the vector's coordinates.
        Vector2 GetNegated() const; // Returns a negated copy of the vector.

        // -- Copy signs -- //
        void    CopySign     (const Vector2& source);       // Copies the signs from the given vector to this vector.
        Vector2 GetCopiedSign(const Vector2& source) const; // Returns a copy of this vector with the given vector's signs.

        // -- Angles -- //
        float GetAngle      () const;                 // Returns the angle (in radians) of the vector with the horizontal axis.
        float GetAngle      (const Vector2& v) const; // Returns the angle (in radians) between this vector and the given one.
        float GetAngleSigned(const Vector2& v) const; // Returns the signed angle (in radians) between this vector and the given one.

        // -- Rotation -- //
        void Rotate       (const float& angle);                       // Rotates the vector by the given angle (in rad).
        void RotateAsPoint(const Vector2& pivot, const float& angle); // Rotates the point around the given pivot point by the given angle (in rad).

        // -- Miscellaneous -- //
        
        // Returns the normal the vector.
        Vector2 GetNormal() const;

        // Interprets the vector as a point and returns the distance to another point.
        float GetDistanceFromPoint(const Vector2& p) const;

        // Calculates linear interpolation for a value from a start point to an end point.
        static Vector2 Lerp(const Vector2& start, const Vector2& dest, const float& val);

        // -- Conversion -- //
        std::string ToString(const int& precision = 2) const; // Returns the vector's contents as a string.
    };
}

#include "Vector2.inl"
