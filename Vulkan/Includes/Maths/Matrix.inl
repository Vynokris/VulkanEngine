#include <cassert>
#include <sstream>
#include <iostream>
#include "Matrix.h"
#include "Arithmetic.h"

// ----- Constructors ----- //

// Default constructor.
template <int R, int C> Maths::Matrix<R, C>::Matrix(const bool& identity)
{
    assert(R >= 2 && C >= 2/*, "Maths::Matrix size is too small."*/);

    if (!identity)
    {
        for (int i = 0; i < R; i++) {
            for (int j = 0; j < C; j++) {
                m[i][j] = 0;
            }
        }
    }
    else
    {
        for (int i = 0; i < R; i++) {
            for (int j = 0; j < C; j++) {
                if (i == j) m[i][j] = 1;
                else        m[i][j] = 0;
            }
        }
    }
}

// Copy operator.
template <int R, int C> Maths::Matrix<R, C>::Matrix(const Matrix<R, C>& matrix)
{
    assert(R >= 2 && C >= 2/*, "Maths::Matrix size is too small."*/);
    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            m[i][j] = matrix[i][j];
}

// Matrix from float 2D array.
template <int R, int C> Maths::Matrix<R, C>::Matrix(const float matrix[R][C])
{
    assert(R > 2 && C > 2/*, "Maths::Matrix size is too small."*/);
    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            m[i][j] = matrix[i][j];
}


// ---------- Operators ---------- //

// Matrix copy.
template <int R, int C> Maths::Matrix<R, C>& Maths::Matrix<R, C>::operator=(const Matrix& matrix)
{
    if (&matrix == this) return *this;

    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            m[i][j] = matrix[i][j];

    return *this;
}

template <int R, int C> Maths::Matrix<R, C>& Maths::Matrix<R, C>::operator=(float** matrix)
{
    assert(sizeof(matrix) / sizeof(float) == R * C/*, "The given matrix is of the wrong size."*/);

    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            m[i][j] = matrix[i][j];

    return *this;
}

// Matrix addition.
template <int R, int C> Maths::Matrix<R, C> Maths::Matrix<R, C>::operator+(const float& val) const
{
    Matrix tmp;
    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            tmp[i][j] = m[i][j] + val;
    return tmp;
}

template <int R, int C> Maths::Matrix<R, C> Maths::Matrix<R, C>::operator+(const Matrix& matrix) const
{
    Matrix tmp;
    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            tmp[i][j] = m[i][j] + matrix[i][j];
    return tmp;
}

// Matrix subtraction and inversion.
template <int R, int C> Maths::Matrix<R, C> Maths::Matrix<R, C>::operator-() const
{
    Matrix tmp;
    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            tmp[i][j] = -m[i][j];
    return tmp;
}

template <int R, int C> Maths::Matrix<R, C> Maths::Matrix<R, C>::operator-(const float& val) const
{
    Matrix tmp;
    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            tmp[i][j] = m[i][j] - val;
    return tmp;
}

template <int R, int C> Maths::Matrix<R, C> Maths::Matrix<R, C>::operator-(const Matrix<R, C>& matrix) const
{
    Matrix tmp;
    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            tmp[i][j] = m[i][j] - matrix[i][j];
    return tmp;
}

// Matrix multiplication.
template <int R, int C> Maths::Matrix<R, C> Maths::Matrix<R, C>::operator*(const float& val) const
{
    Matrix tmp;
    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            tmp[i][j] = m[i][j] * val;
    return tmp;
}

template <int R, int C> template<int R2, int C2>
Maths::Matrix<(R > R2 ? R : R2), (C > C2 ? C : C2)> Maths::Matrix<R, C>::operator*(const Matrix<R2, C2>& matrix) const
{
    assert(C == R2/*, "Given matrices cannot be multiplied."*/);

    Matrix<(R > R2 ? R : R2), (C > C2 ? C : C2)> result;
    for (int i = 0; i < R; i++)
    {
        for (int j = 0; j < C2; j++)
        {
            result[i][j] = 0;
            for (int k = 0; k < R2; k++)
                result[i][j] += m[i][k] * matrix[k][j];
        }
    }
    return result;
}

template <> inline Maths::Vector3 Maths::Matrix<3, 3>::operator*(const Vector3& val) const
{
    return Vector3
    (
        val.x * m[0][0] + val.y * m[1][0] + val.z * m[2][0],
        val.x * m[0][1] + val.y * m[1][1] + val.z * m[2][1],
        val.x * m[0][2] + val.y * m[1][2] + val.z * m[2][2]
    );
}

template <> inline Maths::Vector4 Maths::Matrix<4, 4>::operator*(const Vector4& val) const
{
    return Vector4
    (
        val.x * m[0][0] + val.y * m[1][0] + val.z * m[2][0] + val.w * m[3][0],
        val.x * m[0][1] + val.y * m[1][1] + val.z * m[2][1] + val.w * m[3][1],
        val.x * m[0][2] + val.y * m[1][2] + val.z * m[2][2] + val.w * m[3][2],
        val.x * m[0][3] + val.y * m[1][3] + val.z * m[2][3] + val.w * m[3][3]
    );
}

// Matrix division by a scalar.
template <int R, int C> Maths::Matrix<R, C> Maths::Matrix<R, C>::operator/(const float& val) const
{
    Matrix tmp;
    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            tmp[i][j] = m[i][j] / val;
    return tmp;
}

// Matrix addition assignment.
template <int R, int C> void Maths::Matrix<R, C>::operator+=(const float& val)
{
    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            m[i][j] += val;
}

template <int R, int C> void Maths::Matrix<R, C>::operator+=(const Matrix<R, C>& matrix)
{
    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            m[i][j] += matrix[i][j];
}

// Matrix subtraction assignment.
template <int R, int C> void Maths::Matrix<R, C>::operator-=(const float& val)
{
    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            m[i][j] -= val;
}

template <int R, int C> void Maths::Matrix<R, C>::operator-=(const Matrix& matrix)
{
    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            m[i][j] -= matrix[i][j];
}

// Matrix multiplication assignment.
template <int R, int C> void Maths::Matrix<R, C>::operator*=(const float& val)
{
    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            m[i][j] *= val;
}

template <int R, int C> template <int R2, int C2>
void Maths::Matrix<R, C>::operator*=(const Matrix<R2, C2>& matrix)
{
    *this = *this * matrix;
}

// Matrix power.
template <int R, int C> Maths::Matrix<R, C> Maths::Matrix<R, C>::Pow(const float& n) const
{
    Matrix tmp;
    for (int n0 = 0; n0 < n; n0++)
        for (int i = 0; i < R; i++)
            for (int j = 0; j < C; j++)
                tmp[i][j] *= m[i][j];
    return tmp;
}


// ---------- Methods ---------- //

// Getters.
template <int R, int C> const float* Maths::Matrix<R, C>::AsPtr() const
{
    return &m[0][0];
}

template <int R, int C> float* Maths::Matrix<R, C>::AsPtr()
{
    return &m[0][0];
}

template <int R, int C> bool Maths::Matrix<R, C>::IsIdentity() const
{
    for (int i = 0; i < R; i++) {
        for (int j = 0; j < C; j++) {
            if ((i != j && m[i][j] != 0) || (i == j && m[i][j] != 1))
                return false;
        }
    }
    return true;
}

template <int R, int C> bool Maths::Matrix<R, C>::IsSymmetrical() const
{
    for (int i = 0; i < R; i++) {
        for (int j = i+1; j < C; j++) {
            if (roundInt(m[i][j]*10000)/10000.f != roundInt(m[j][i]*10000)/10000.f) // Round to 5 floating points.
                return false;
        }
    }
    return true;
}

// Transposition.
template <int R, int C> void Maths::Matrix<R, C>::Transpose()
{
    *this = GetTransposed();
}

template <int R, int C> Maths::Matrix<R, C> Maths::Matrix<R, C>::GetTransposed() const
{
    Matrix<C, R> result;
    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            result[j][i] = m[i][j];
    return result;
}

// Utilities.
template <int R, int C> void Maths::Matrix<R, C>::RemoveScale()
{
    *this = GetWithoutScale();
}

template <int R, int C> Maths::Matrix<R, C> Maths::Matrix<R, C>::GetWithoutScale() const
{
    Matrix mat = *this;
    for (int i = 0; i < R; i++)
    {
        float scale = 0;
        for (int j = 0; j < C; j++)
            scale += sqpow(mat[i][j]);
        scale = sqrt(scale);
        for (int j = 0; j < C; j++)
            mat[i][j] /= scale;
    }
    return mat;
}

// Conversions.
template <int R, int C> std::string Maths::Matrix<R, C>::ToString(const int& precision) const
{
    std::ostringstream output;

    for (int i = 0; i < R; i++)
    {
        for (int j = 0; j < C; j++)
        {
            output.precision(precision);
            output << std::fixed << m[i][j];
            if (i < R-1 || j < C-1)
                output << ", ";
        }
        if (i < R-1)
            output << std::endl;
    }
    return output.str();
}
