#pragma once
#include "Vector3.h"
#include "Vector2.h"
#include "Matrix.h"

namespace Maths
{
    // A point in 3D space with rendering data.
    struct Vertex
    {
        Vector3 pos;
        Vector2 uv;
        Vector3 normal;

        bool operator==(const Vertex& other) const
        {
            return pos    == other.pos
                && uv     == other.uv
                && normal == other.normal;
        }
    };

    // A point in 3D space with texture coordinates and tangent-space vectors.
    struct TangentVertex
    {
        Vector3 pos;
        Vector2 uv;
        Vector3 normal;
        Vector3 tangent;
        Vector3 bitangent;

        bool operator==(const TangentVertex& other) const
        {
            return pos       == other.pos
                && uv        == other.uv
                && normal    == other.normal
                && tangent   == other.tangent
                && bitangent == other.bitangent;
        }
    };

    // Holds indices for the vertex's data.
    struct VertexIndices
    {
        uint32_t pos;
        uint32_t uv;
        uint32_t normal;
    };

    // Holds model, view and projection matrices.
    struct MvpBuffer {
        Mat4 model;
        Mat4 view;
        Mat4 proj;
    };
}

// Define hashing methods for Maths classes.
namespace std
{
    template<> struct hash<Maths::Vector2>
    {
        size_t operator()(Maths::Vector2 const& vec) const noexcept
        {
            return hash<float>()(vec.x) ^ (hash<float>()(vec.y) << 1);
        }
    };
    
    template<> struct hash<Maths::Vector3>
    {
        size_t operator()(Maths::Vector3 const& vec) const noexcept
        {
            return ((hash<float>()(vec.x) ^
                    (hash<float>()(vec.y) << 1)) >> 1) ^
                    (hash<float>()(vec.z) << 1);
        }
    };
    
    template<> struct hash<Maths::Vertex>
    {
        size_t operator()(Maths::Vertex const& vertex) const noexcept
        {
            return ((hash<Maths::Vector3>()(vertex.pos) ^
                    (hash<Maths::Vector2>()(vertex.uv) << 1)) >> 1) ^
                    (hash<Maths::Vector3>()(vertex.normal) << 1);
        }
    };
    
    template<> struct hash<Maths::TangentVertex>
    {
        size_t operator()(Maths::TangentVertex const& vertex) const noexcept
        {
            return ((hash<Maths::Vector3>()(vertex.pos) ^
                    (hash<Maths::Vector2>()(vertex.uv) << 1)) >> 1) ^
                    (hash<Maths::Vector3>()(vertex.normal) << 1);
        }
    };
}
