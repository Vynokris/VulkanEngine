#pragma once
#include <string>

namespace Maths
{
    class Vector3;
    class Vector4;
    class AngleAxis;
    class Quaternion;
    
    // - Matrix<R, C>: a matrix with R rows and C columns - //
    template<int R, int C> class Matrix;

    // -- Shortcuts for square matrices -- //
    typedef Matrix<2, 2> Mat2; // 2x2 matrix.
    typedef Matrix<3, 3> Mat3; // 3x3 matrix.
    typedef Matrix<4, 4> Mat4; // 4x4 matrix.
    
    template<int R, int C> class Matrix
    {
    public:
        float m[R][C];
        
        // -- Constructors -- //
        
        Matrix(const bool& identity = true); // Default constructor.
        Matrix(const Matrix& matrix);        // Copy operator.
        Matrix(const float matrix[R][C]);    // Matrix from float 2D array.
        
        Matrix(const Vector3&    eulerAngles); // Matrix from euler angles (pitch, roll, yaw).
        Matrix(const AngleAxis&  angleAxis  ); // Matrix from angle-axis.
        Matrix(const Quaternion& quaternion ); // Matrix from quaternion.
        
        // Matrix 2x2 constructor.
        Matrix(const float& m00, const float& m01, 
               const float& m10, const float& m11);

        // Matrix 3x3 constructor.
        Matrix(const float& m00, const float& m01, const float& m02,
               const float& m10, const float& m11, const float& m12,
               const float& m20, const float& m21, const float& m22);

        // Matrix 4x4 constructor.
        Matrix(const float& m00, const float& m01, const float& m02, const float& m03,
               const float& m10, const float& m11, const float& m12, const float& m13,
               const float& m20, const float& m21, const float& m22, const float& m23,
               const float& m30, const float& m31, const float& m32, const float& m33);

        // Matrix 4x4 constructor from 2x2 matrices.
        Matrix(const Mat2& a, const Mat2& b, const Mat2& c, const Mat2& d);


        // -- Static constructors -- //

        static Matrix<4, 4> FromTranslation(const Vector3&    translation); // Matrix from translation.
        static Matrix<4, 4> FromPitch      (const float&      angle      ); // Matrix that rotates around the X axis by the given angle.
        static Matrix<4, 4> FromRoll       (const float&      angle      ); // Matrix that rotates around the Y axis by the given angle.
        static Matrix<4, 4> FromYaw        (const float&      angle      ); // Matrix that rotates around the Z axis by the given angle.
        static Matrix<4, 4> FromEuler      (const Vector3&    angles     ); // Matrix from euler angles (pitch, roll, yaw).
        static Matrix<4, 4> FromAngleAxis  (const AngleAxis&  angleAxis  ); // Matrix from angle-axis.
        static Matrix<4, 4> FromQuaternion (const Quaternion& quaternion ); // Matrix from quaternion.
        static Matrix<4, 4> FromScale      (const Vector3&    scale      ); // Matrix from scale.
        static Matrix<4, 4> FromTransform  (const Vector3& pos, const Quaternion& rot, const Vector3& scale, const bool& reverse = false); // Matrix from transform.
        
        // -- Operators -- //

        // Matrix bracket operators.
        const float* operator[](int index) const { return m[index]; }
              float* operator[](int index)       { return m[index]; }

        // Matrix copy.
        Matrix& operator=(const Matrix& matrix);
        Matrix& operator=(float** matrix);

        // Matrix addition.
        Matrix operator+(const float& val) const;
        Matrix operator+(const Matrix& matrix) const;

        // Matrix negation and subtraction.
        Matrix operator-() const;
        Matrix operator-(const float& val) const;
        Matrix operator-(const Matrix& matrix) const;
        
        // Matrix multiplication.
        Matrix  operator*(const float&   val) const;
        Vector3 operator*(const Vector3& val) const;
        Vector4 operator*(const Vector4& val) const;
        template<int R2, int C2> Matrix<(R > R2 ? R : R2), (C > C2 ? C : C2)> operator*(const Matrix<R2, C2>& matrix) const;

        // Matrix division by a scalar.
        Matrix operator/(const float& val) const;

        // Matrix addition assignment.
        void operator+=(const float& val);
        void operator+=(const Matrix& matrix);

        // Matrix subtraction assignment.
        void operator-=(const float& val);
        void operator-=(const Matrix& matrix);

        // Matrix multiplication assignment.
        void operator*=(const float& val);
        template<int R2, int C2> void operator*=(const Matrix<R2, C2>& matrix);

        // Matrix power.
        Matrix Pow(const float& n) const; // Returns this matrix raised to the given power.

        // -- Getters -- //
        const float* AsPtr() const;                                   // Returns a constant pointer to the matrix's float array.
        float* AsPtr();                                               // Returns a pointer to the matrix's float array.
        int    GetRows()                    const { return R; }       // Returns the number of rows in the matrix.
        int    GetColumns()                 const { return C; }       // Returns the number of columns in the matrix.
        float  GetMatrixValue(int i, int j) const { return m[i][j]; } // Returns the element at the given indices.
        bool   IsSquare()                   const { return R == C; }  // Returns true if the matrix is square, false if not.
        bool   IsIdentity()    const;                                 // Returns true if the matrix is identity.
        bool   IsSymmetrical() const;                                 // Returns true if the matrix is symmetrical.

        // -- Determinants -- //
        float Det2() const; // Returns a 2x2 matrix's determinant.
        float Det3() const; // Returns a 3x3 matrix's determinant.
        float Det4() const; // Returns a 4x4 matrix's determinant.

        // -- Inverses -- //
        Mat2 Inv2() const; // Returns the inverse of a 2x2 matrix's.
        Mat3 Inv3() const; // Returns the inverse of a 3x3 matrix's.
        Mat4 Inv4() const; // Returns the inverse of a 4x4 matrix's.

        // -- Transposition -- //
        void   Transpose    ();       // Transposes the matrix.
        Matrix GetTransposed() const; // Returns a transposed copy of this matrix.

        // -- Utilities -- //
        void   RemoveScale();           // Removes scaling form this matrix.
        Matrix GetWithoutScale() const; // Returns a copy of this matrix without any scaling.
        
        // -- Conversions -- //
        Matrix<4, 4> ToMat4      () const;
        Matrix<3, 3> ToMat3      () const;
        Vector3      ToPosition  () const;                     // Extracts the position from a transformation matrix (must be 4x4).
        Quaternion   ToQuaternion() const;                     // Conversion to quaternion (must be 3x3 or 4x4).
        AngleAxis    ToAngleAxis () const;                     // Conversion to angle axis rotation (must be 3x3 or 4x4).
        Vector3      ToScale     () const;                     // Extracts the scale from a transformation matrix (must be 3x3 or 4x4).
        std::string  ToString(const int& precision = 2) const; // Returns matrix contents as string.
    };
}

#include "Matrix.inl"
