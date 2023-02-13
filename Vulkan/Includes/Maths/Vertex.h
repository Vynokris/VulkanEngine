#pragma once

#include "Vector3.h"
#include "Vector2.h"

namespace Maths
{
    // Structure used for the vulkan tutorial.
    struct TestVertex
    {
        Vector3 pos;
        Vector3 color;
        Vector2 texCoord;

        bool operator==(const TestVertex& other) const
        {
            return pos      == other.pos
                && color    == other.color
                && texCoord == other.texCoord;
        }
    };
    
    // A point in 3D space with rendering data.
    struct Vertex
    {
        Vector3 pos;
        Vector2 uv;
        Vector3 normal;
    };

    // Holds indices for the vertex's data.
    struct TangentVertex
    {
        Vector3 pos;
        Vector2 uv;
        Vector3 normal;
        Vector3 tangent;
        Vector3 bitangent;
    };

    struct VertexIndices
    {
        uint32_t pos;
        uint32_t uv;
        uint32_t normal;
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
    
    template<> struct hash<Maths::TestVertex>
    {
        size_t operator()(Maths::TestVertex const& vertex) const noexcept
        {
            return ((hash<Maths::Vector3>()(vertex.pos) ^
                    (hash<Maths::Vector3>()(vertex.color) << 1)) >> 1) ^
                    (hash<Maths::Vector2>()(vertex.texCoord) << 1);
        }
    };
}
