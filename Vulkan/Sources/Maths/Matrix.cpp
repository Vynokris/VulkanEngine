#include "Maths/Maths.h"

// ----- Constructors ----- //

// Matrix 2x2 constructor.
template <> Maths::Matrix<2, 2>::Matrix(const float& m00, const float& m01, const float& m10, const float& m11)
{
    m[0][0] = m00; m[0][1] = m01;
    m[1][0] = m10; m[1][1] = m11;
}

// Matrix 3x3 constructor.
template <> Maths::Matrix<3, 3>::Matrix(const float& m00, const float& m01, const float& m02, 
                                        const float& m10, const float& m11, const float& m12, 
                                        const float& m20, const float& m21, const float& m22)
{
    m[0][0] = m00; m[0][1] = m01; m[0][2] = m02;
    m[1][0] = m10; m[1][1] = m11; m[1][2] = m12;
    m[2][0] = m20; m[2][1] = m21; m[2][2] = m22;
}

// Matrix 4x4 constructor.
template <> Maths::Matrix<4, 4>::Matrix(const float& m00, const float& m01, const float& m02, const float& m03, 
                                        const float& m10, const float& m11, const float& m12, const float& m13, 
                                        const float& m20, const float& m21, const float& m22, const float& m23, 
                                        const float& m30, const float& m31, const float& m32, const float& m33)
{
    m[0][0] = m00; m[0][1] = m01; m[0][2] = m02; m[0][3] = m03;
    m[1][0] = m10; m[1][1] = m11; m[1][2] = m12; m[1][3] = m13;
    m[2][0] = m20; m[2][1] = m21; m[2][2] = m22; m[2][3] = m23;
    m[3][0] = m30; m[3][1] = m31; m[3][2] = m32; m[3][3] = m33;
}

// Matrix 4x4 constructor (from 2x2 matrices).
template <> Maths::Matrix<4, 4>::Matrix(const Mat2& a, const Mat2& b, const Mat2& c, const Mat2& d)
{
    m[0][0] = a[0][0]; m[0][1] = a[0][1]; m[0][2] = b[0][0]; m[0][3] = b[0][1];
    m[1][0] = a[1][0]; m[1][1] = a[1][1]; m[1][2] = b[1][0]; m[1][3] = b[1][1];
    m[2][0] = c[0][0]; m[2][1] = c[0][1]; m[2][2] = d[0][0]; m[2][3] = d[0][1];
    m[3][0] = c[1][0]; m[3][1] = c[1][1]; m[3][2] = d[1][0]; m[3][3] = d[1][1];
}

// Matrix from euler angles.
template <> Maths::Matrix<4, 4>::Matrix(const Vector3& eulerAngles)
{
    *this = Quaternion::FromEuler(eulerAngles).ToMatrix();
}

// Matrix from angle-axis.
template <> Maths::Matrix<4, 4>::Matrix(const AngleAxis& angleAxis)
{
    *this = angleAxis.ToMatrix();
}

// Matrix from quaternion.
template <> Maths::Matrix<4, 4>::Matrix(const Quaternion& quaternion)
{
    *this = quaternion.ToMatrix();
}


// ----- Static constructors ----- //

// Translation matrix.
template <> Maths::Matrix<4, 4> Maths::Matrix<4, 4>::FromTranslation(const Vector3& translation)
{
    return Mat4(1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                translation.x, translation.y, translation.z, 1);
}

// Rotation matrices.
template <> Maths::Matrix<4, 4> Maths::Matrix<4, 4>::FromPitch     (const float&      angle     ) { return AngleAxis(angle, { 1, 0, 0 }).ToMatrix(); }
template <> Maths::Matrix<4, 4> Maths::Matrix<4, 4>::FromRoll      (const float&      angle     ) { return AngleAxis(angle, { 0, 1, 0 }).ToMatrix(); }
template <> Maths::Matrix<4, 4> Maths::Matrix<4, 4>::FromYaw       (const float&      angle     ) { return AngleAxis(angle, { 0, 0, 1 }).ToMatrix(); }
template <> Maths::Matrix<4, 4> Maths::Matrix<4, 4>::FromEuler     (const Vector3&    angles    ) { return Quaternion::FromEuler(angles).ToMatrix(); }
template <> Maths::Matrix<4, 4> Maths::Matrix<4, 4>::FromAngleAxis (const AngleAxis&  angleAxis ) { return angleAxis .ToMatrix(); }
template <> Maths::Matrix<4, 4> Maths::Matrix<4, 4>::FromQuaternion(const Quaternion& quaternion) { return quaternion.ToMatrix(); }

// Scale matrix.
template <> Maths::Matrix<4, 4> Maths::Matrix<4, 4>::FromScale(const Vector3& scale)
{
    return Mat4(scale.x, 0, 0, 0,
                0, scale.y, 0, 0,
                0, 0, scale.z, 0,
                0, 0, 0, 1);
}

// Transform matrix.
template <> Maths::Matrix<4, 4> Maths::Matrix<4, 4>::FromTransform(const Vector3& pos, const Quaternion& rot, const Vector3& scale, const bool& reverse)
{
    if (reverse)
    {
        return FromTranslation(pos)          *
               rot.GetConjugate().ToMatrix() *
               FromScale(scale);
    }
    return FromScale(scale) *
           rot.ToMatrix()   * 
           FromTranslation(pos);
}


// ----- Methods ----- //

// Determinants.
template <> float Maths::Matrix<2, 2>::Det2() const
{
    return m[0][0] * m[1][1] - m[0][1] * m[1][0];
}

template <> float Maths::Matrix<3, 3>::Det3() const
{
    return m[0][0] * Mat2(m[1][1], m[1][2], m[2][1], m[2][2]).Det2() -
         - m[0][1] * Mat2(m[1][0], m[1][2], m[2][0], m[2][2]).Det2() +
         + m[0][2] * Mat2(m[1][0], m[1][1], m[2][0], m[2][1]).Det2();
}

template <> float Maths::Matrix<4, 4>::Det4() const
{
    const Mat3 a(m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3], m[3][1], m[3][2], m[3][3]);
    const Mat3 b(m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3], m[3][0], m[3][2], m[3][3]);
    const Mat3 c(m[1][0], m[1][1], m[1][3], m[2][0], m[2][1], m[2][3], m[3][0], m[3][1], m[3][3]);
    const Mat3 d(m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2], m[3][0], m[3][1], m[3][2]);

    return (a.Det3() * m[0][0] - b.Det3() * m[0][1] + c.Det3() * m[0][2] - d.Det3() * m[0][3]);
}

// Inverses.
template <> Maths::Mat2 Maths::Matrix<2, 2>::Inv2() const
{
    const Mat2 val(m[1][1], -m[0][1], -m[1][0], m[0][0]);
    return val / val.Det2();
}

template <> Maths::Mat4 Maths::Matrix<4, 4>::Inv4() const
{
    const Mat2 a(m[0][0], m[0][1], m[1][0], m[1][1]);
    const Mat2 b(m[0][2], m[0][3], m[1][2], m[1][3]);
    const Mat2 c(m[2][0], m[2][1], m[3][0], m[3][1]);
    const Mat2 d(m[2][2], m[2][3], m[3][2], m[3][3]);

    return Mat4((a - b * d.Inv2() * c).Inv2(), -(a - b * d.Inv2() * c).Inv2() * b * d.Inv2(),
               -(d - c * a.Inv2() * b).Inv2() * c * a.Inv2(), (d - c * a.Inv2() * b).Inv2());
}

template <> Maths::Mat3 Maths::Matrix<3, 3>::Inv3() const
{
    Mat4 val(m[0][0], m[0][1], m[0][2], 0,
             m[1][0], m[1][1], m[1][2], 0,
             m[2][0], m[2][1], m[2][2], 0,
             0,       0,       0,       1);
    
    val = val.Inv4();
    
    return Mat3(val[0][0], val[0][1], val[0][2],
                val[1][0], val[1][1], val[1][2],
                val[2][0], val[2][1], val[2][2]);
}

// Conversions.
template <> Maths::Matrix<4, 4> Maths::Matrix<3, 3>::ToMat4() const
{
    return Mat4(m[0][0], m[0][1], m[0][2], 0,
                m[1][0], m[1][1], m[1][2], 0,
                m[2][0], m[2][1], m[2][2], 0,
                0,       0,       0,       1);
}

template <> Maths::Matrix<3, 3> Maths::Matrix<4, 4>::ToMat3() const
{
    return Mat3(m[0][0], m[0][1], m[0][2],
                m[1][0], m[1][1], m[1][2],
                m[2][0], m[2][1], m[2][2]);
}
template <> Maths::Vector3 Maths::Matrix<4, 4>::ToPosition() const
{
    return { m[3][0], m[3][1], m[3][2] };
}

template <> Maths::Quaternion Maths::Matrix<3, 3>::ToQuaternion() const
{
    Quaternion quaternion;
    quaternion.w = sqrt(std::max(0.f, 1 + m[0][0] + m[1][1] + m[2][2])) * 0.5f; 
    quaternion.x = sqrt(std::max(0.f, 1 + m[0][0] - m[1][1] - m[2][2])) * 0.5f; 
    quaternion.y = sqrt(std::max(0.f, 1 - m[0][0] + m[1][1] - m[2][2])) * 0.5f; 
    quaternion.z = sqrt(std::max(0.f, 1 - m[0][0] - m[1][1] + m[2][2])) * 0.5f; 
    quaternion.x = copysign(quaternion.x, m[2][1] - m[1][2]); 
    quaternion.y = copysign(quaternion.y, m[0][2] - m[2][0]); 
    quaternion.z = copysign(quaternion.z, m[1][0] - m[0][1]);
    return quaternion;
} 
template <> Maths::Quaternion Maths::Matrix<4, 4>::ToQuaternion() const
{
    return (*this).ToMat3().ToQuaternion();
}

template <> Maths::AngleAxis Maths::Matrix<3, 3>::ToAngleAxis() const
{
    const float angle = acos((m[0][0] + m[1][1] + m[2][2] - 1) * 0.5f);
    const float n     = 2 * sin(angle);
    
    AngleAxis angleAxis;
    if (angle == 0.f || angle == PI) angleAxis = AngleAxis(angle, Vector3(m[1][2]-m[2][1], m[2][0]-m[0][2], m[0][1]-m[1][0]).GetNormalized().GetNegated());
    else                             angleAxis = AngleAxis(angle, Vector3(m[1][2]-m[2][1], m[2][0]-m[0][2], m[0][1]-m[1][0])/n);
    return angleAxis;
}
template <> Maths::AngleAxis Maths::Matrix<4, 4>::ToAngleAxis() const
{
    return (*this).ToMat3().ToAngleAxis();
}

template <> Maths::Vector3 Maths::Matrix<3, 3>::ToScale() const
{
    Vector3 scale;
    for (int i = 0; i < 3; i++)
        *(&scale.x+i) = Vector3(m[0][i], m[1][i], m[2][i]).GetLength();
    return scale;
}
template <> Maths::Vector3 Maths::Matrix<4, 4>::ToScale() const
{
    return (*this).ToMat3().ToScale();
}
