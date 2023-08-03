#pragma once
#include "Maths/Vertex.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace Resources { class Model; class Mesh; class Material; }
namespace Core
{
    class Engine;
    
    // - WavefrontParser: Custom OBJ and MTL loader - //
    class WavefrontParser
    {
    private:
        inline static Engine* engine = nullptr;
        
    public:
        WavefrontParser()                                  = delete;
        WavefrontParser(const WavefrontParser&)            = delete;
        WavefrontParser(WavefrontParser&&)                 = delete;
        WavefrontParser& operator=(const WavefrontParser&) = delete;
        WavefrontParser& operator=(WavefrontParser&&)      = delete;
        ~WavefrontParser()                                 = delete;
        
        // -- Static Methods -- //
        static std::unordered_map<std::string, Resources::Material> ParseMtl(const std::string& filename); // Loads materials and textures from the specified MTL file.
        static std::unordered_map<std::string, Resources::Model*  > ParseObj(const std::string& filename); // Loads models, meshes and materials from the specified OBJ file.
    
    private:
        static void                 ParseMtlColor       (const std::string& line, float* colorValues);
        static void                 ParseObjVertexValues(const std::string& line, std::vector<float>& values, const int& startIndex, const int& valCount);
        static Maths::VertexIndices ParseObjIndices     (std::string indicesStr);
        static void                 ParseObjTriangle    (const std::string& line, std::array<std::vector<uint32_t>, 3>& indices);
        
        static void ParseObjObjectLine  (                             const std::string& line, Resources::Model*& model, std::unordered_map<std::string, Resources::Model*>& newModels);
        static void ParseObjGroupLine   (const std::string& filename, const std::string& line, Resources::Model*& model, std::unordered_map<std::string, Resources::Model*>& newModels);
        static void ParseObjUsemtlLine  (const std::string& filename, const std::string& line, Resources::Model*& model, std::unordered_map<std::string, Resources::Model*>& newModels, std::unordered_map<std::string, Resources::Material>& newMaterials);
        static void ParseObjIndicesLine (const std::string& filename, const std::string& line, std::stringstream& fileContents, Resources::Model*& model, std::unordered_map<std::string, Resources::Model*>& newModels, const std::array<std::vector<float>, 3>& vertexData);
        static std::array<std::vector<uint32_t>, 3> ParseObjMeshIndices(std::stringstream& fileContents);
        static void ParseObjMeshVertices(Resources::Mesh* mesh, const std::array<std::vector<float>, 3>& vertexData, const std::array<std::vector<uint32_t>, 3>& vertexIndices);
    };
}
