#include "Core/WavefrontParser.h"
#include "Core/Application.h"
#include "Core/Engine.h"
#include "Resources/Model.h"
#include "Resources/Mesh.h"
#include "Resources/Material.h"
#include "Resources/Texture.h"
#include "Maths/Vertex.h"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;
namespace cr = std::chrono;
using namespace Core;
using namespace Resources;
using namespace Maths;

std::unordered_map<std::string, Material> WavefrontParser::ParseMtl(const std::string& filename)
{
    if (engine == nullptr) engine = Application::Get()->GetEngine();
    
    // Read file contents and extract them to the data string.
    std::stringstream fileContents;
    {
        std::fstream f(filename, std::ios_base::in | std::ios_base::app);
        fileContents << f.rdbuf();
        f.close();
    }
    std::string filepath = fs::path(filename).parent_path().string() + "\\";
    std::unordered_map<std::string, Material> newMaterials;

    // Read file line by line to create material data.
    std::string line;
    std::string curMatName;
    while (std::getline(fileContents, line)) 
    {
        // Create new material.
        if (line[0] == 'n' && line[1] == 'e' && line[2] == 'w')
        {
            curMatName = line.substr(7, line.size() - 7);
            newMaterials[curMatName] = Material();
            newMaterials[curMatName].name = curMatName;
            continue;
        }

        // Don't touch material values if it is not yet created.
        if (curMatName.empty())
            continue;
        line += ' ';

        // Read color values.
        if (line[0] == 'K')
        {
            switch (line[1])
            {
            // case 'a':
            //     ParseMtlColor(line, newMaterials[curMatName]->ambient.ptr());
            //     continue;
            case 'd':
                ParseMtlColor(line, newMaterials[curMatName].albedo.AsPtr());
                continue;
            // case 's':
            //     ParseMtlColor(line, newMaterials[curMatName].specular.ptr());
            //     continue;
            case 'e':
                ParseMtlColor(line, newMaterials[curMatName].emissive.AsPtr());
                continue;
            default:
                break;
            }
        }

        // Read shininess.
        if (line[0] == 'N' && line[1] == 's')
        {
            newMaterials[curMatName].shininess = std::strtof(line.substr(3, line.size()-4).c_str(), nullptr);
            continue;
        }

        // Read transparency.
        if (line[0] == 'd')
        {
            newMaterials[curMatName].alpha = std::strtof(line.substr(2, line.size()-3).c_str(), nullptr);
            continue;
        }
        if (line[0] == 'T' && line[1] == 'r')
        {
            newMaterials[curMatName].alpha = 1 - std::strtof(line.substr(2, line.size()-3).c_str(), nullptr);
            continue;
        }

        // Read texture maps.
        if (line[0] == 'm' && line[1] == 'a' && line[2] == 'p')
        {
            if (line[4] == 'K')
            {
                std::string texName = line.substr(7, line.size()-8);
                std::string texPath = filepath + texName;
                engine->LoadFile(texPath);
                switch (line[5])
                {
                    // Ambient texture.
                    /*
                    case 'a':
                        newMaterials[curMatName].ambientTexture = engine->GetTexture(texPath);
                    continue;
                    */

                    // Diffuse texture.
                    case 'd':
                        newMaterials[curMatName].albedoTexture = engine->GetTexture(texPath);
                    continue;

                    // Specular texture.
                    /*
                    case 's':
                        newMaterials[curMatName].specularTexture = engine->GetTexture(texPath);
                    continue;
                    */

                    // Emission texture.
                    case 'e':
                        newMaterials[curMatName].emissiveTexture = engine->GetTexture(texPath);
                    continue;

                default:
                    break;
                }
            }
            // Shininess map.
            if (line[4] == 'N' && line[5] == 's')
            {
                const std::string texName = line.substr(7, line.size()-8);
                std::string texPath = filepath + texName;
                engine->LoadFile(texPath);
                newMaterials[curMatName].shininessMap = engine->GetTexture(texPath);
                continue;
            }
            // Alpha map.
            if (line[4] == 'd')
            {
                const std::string texName = line.substr(6, line.size()-7);
                std::string texPath = filepath + texName;
                engine->LoadFile(texPath);
                newMaterials[curMatName].alphaMap = engine->GetTexture(texPath);
                continue;
            }
        }
        /*
        {
            // Normal map.
            size_t bumpIndex = line.find("bump ");
            if (bumpIndex == std::string::npos) bumpIndex = line.find("Bump ");
            if (bumpIndex != std::string::npos)
            {
                std::string texName = line.substr(bumpIndex+5, line.size()-(bumpIndex+5)-1);
                std::string texPath = filepath + texName;
                engine->LoadFile(texPath);
                newMaterials[curMatName].normalMap = engine->GetTexture(texPath);
                continue;
            }
        }
        */
    }
    
    return newMaterials;
}

std::unordered_map<std::string, Model*> WavefrontParser::ParseObj(const std::string& filename)
{
    if (!engine) engine = Application::Get()->GetEngine();

    // Start chrono.
    cr::steady_clock::time_point chronoStart = cr::high_resolution_clock::now();

    // Read file contents and extract them to the data string.
    std::stringstream fileContents;
    {
        std::fstream f(filename, std::ios_base::in | std::ios_base::app);
        fileContents << f.rdbuf();
        f.close();
    }
    std::string filepath = fs::path(filename).parent_path().string() + "\\";
    std::unordered_map<std::string, Model*  > newModels    = {};
    std::unordered_map<std::string, Material> newMaterials = {};

    // Define dynamic arrays for positions, uvs, and normals.
    // Format: vertexData[0] = positions, vertexData[1] = uvs, vertexData[2] = normals.
    std::array<std::vector<float>, 3> vertexData;

    // Store a pointer to the mesh group that is currently being created.
    Model* model = nullptr;

    // Read file line by line to create vertex data.
    std::string line;
    while (std::getline(fileContents, line))
    {
        line += ' ';
        switch (line[0])
        {
        case 'v':
            switch (line[1])
            {
                // Parse vertex coords and normals.
                case ' ':
                    ParseObjVertexValues(line, vertexData[0], 2, 3);
                break;
                // Parse vertex UVs.
                case 't':
                    ParseObjVertexValues(line, vertexData[1], 3, 2);
                break;
                // Parse vertex normals.
                case 'n':
                    ParseObjVertexValues(line, vertexData[2], 3, 3);
                break;
        default:
            break;
            }
            break;

        case 'o':
            ParseObjObjectLine(line, model, newModels);
            break;

        case 'g':
            ParseObjGroupLine(filename, line, model, newModels);
            break;

        case 'm':
            newMaterials = ParseMtl(filepath + line.substr(7, line.size() - 8));
            break;

        case 'u':
            ParseObjUsemtlLine(filename, line, model, newModels, newMaterials);
            break;

        case 'f':
            ParseObjIndicesLine(filename, line, fileContents, model, newModels, vertexData);
            break;

        default:
            break;
        }
    }
    if (model == nullptr || model->meshes.empty()) {
        std::cout << "ERROR (Wavefront): Mesh has no sub-meshes after being loaded from obj file " << filename << std::endl;
    }
    else {
        model->meshes.back().FinalizeLoading();
    }

    // End chrono.
    cr::steady_clock::time_point chronoEnd = cr::high_resolution_clock::now();
    cr::nanoseconds elapsed = cr::duration_cast<cr::nanoseconds>(chronoEnd - chronoStart);
    std::cout << "Loading file " << filename << " took " << std::to_string((double)elapsed.count() * 1e-9) << " seconds." << std::endl;

    return newModels;
}

#pragma region Value Parsing
void WavefrontParser::ParseMtlColor(const std::string& line, float* colorValues)
{
    size_t start = 3;
    size_t end = line.find(' ', start);

    for (int i = 0; i < 3; i++)
    {
        // Find the current coordinate value.
        std::string val = line.substr(start, end - start);

        // Set the current color value.
        colorValues[i] = std::strtof(val.c_str(), nullptr);

        // Update the start and end indices.
        start = end + 1;
        end = line.find(' ', start);
    }
}

void WavefrontParser::ParseObjVertexValues(const std::string& line, std::vector<float>& values, const int& startIndex, const int& valCount)
{
    // Find the start and end of the first value.
    size_t start = startIndex;
    size_t end = line.find(' ', start);
    if (end == start) {
        start++;
        end = line.find(' ', start);
    }

    for (int i = 0; i < valCount; i++)
    {
        // Find the current coordinate value.
        std::string val = line.substr(start, end - start);

        // Set the current value.
        values.push_back(std::strtof(val.c_str(), nullptr));

        // Update the start and end indices.
        start = end + 1;
        end = line.find(' ', start);
    }
}

VertexIndices WavefrontParser::ParseObjIndices(std::string indicesStr)
{
    VertexIndices vertex = { 0, 0, 0 };
    size_t start = 0;
    size_t end = indicesStr.find('/', start);
    for (int i = 0; i < 3 && indicesStr[start] != '\0'; i++)
    {
        // Find the current coordinate value.
        std::string index = indicesStr.substr(start, end - start);

        // Set the current index and skip double slashes.
        if (start != end)
            *(&vertex.pos + i) = std::atoi(index.c_str()) - 1;

        // Update the start and end indices.
        start = end + 1;
        end = indicesStr.find('/', start);
    }
    return vertex;
}

void WavefrontParser::ParseObjTriangle(const std::string& line, std::array<std::vector<uint32_t>, 3>& indices)
{
    size_t start = 2;
    size_t end = line.find(' ', start);
    if (end == start) {
        start++;
        end = line.find(' ', start);
    }

    for (int i = 0; i < 3; i++)
    {
        // Find the current coordinate value.
        const std::string indicesStr = line.substr(start, end - start) + '/';

        // Set the current index.
        VertexIndices vertexIndices = ParseObjIndices(indicesStr);
        for (size_t j = 0; j < 3; j++)
            indices[j].push_back(*((&vertexIndices.pos) + j));

        // Update the start and end line of the coord val.
        start = end + 1;
        end = line.find(' ', start);
    }
}
#pragma endregion 

#pragma region Line Parsing
void WavefrontParser::ParseObjObjectLine(const std::string& line, Model*& model, std::unordered_map<std::string, Model*>& newModels)
{
    model = new Model(line.substr(2, line.size()-3));
    newModels[model->GetName()] = model;
}

void WavefrontParser::ParseObjGroupLine(const std::string& filename, const std::string& line, Model*& model, std::unordered_map<std::string, Model*>& newModels)
{
    if (line[2] == '\0')
        return;

    // Make sure a model was already created.
    if (model == nullptr) {
        model = new Model("model_" + fs::path(filename).stem().string());
        newModels[model->GetName()] = model;
    }

    // Finalize loading of the previous mesh.
    if (!model->meshes.empty())
        model->meshes.back().FinalizeLoading();

    // Create a sub-mesh and add it to the model.
    model->meshes.emplace_back(line.substr(2, line.size() - 3), *model);
}

void WavefrontParser::ParseObjUsemtlLine(const std::string& filename, const std::string& line, Model*& model, std::unordered_map<std::string, Model*>& newModels, std::unordered_map<std::string, Material>& newMaterials)
{
    // Make sure a model was already created.
    if (model == nullptr) {
        model = new Model("model_" + fs::path(filename).stem().string());
        newModels[model->GetName()] = model;
    }

    // Make sure a mesh was already created.
    if (model->meshes.empty()) {
        model->meshes.emplace_back("mesh_" + line.substr(7, line.size() - 8), *model);
    }

    // Make sure the current mesh doesn't already have a material.
    else if (!model->meshes.back().name.empty()) {
        model->meshes.back().FinalizeLoading();
        model->meshes.emplace_back("mesh_" + line.substr(7, line.size() - 8), *model);
    }

    // Set the current mesh's material.
    model->meshes.back().SetMaterial(newMaterials[line.substr(7, line.size() - 8)]);
}

void WavefrontParser::ParseObjIndicesLine(const std::string& filename, const std::string& line, std::stringstream& fileContents, Model*& model, std::unordered_map<std::string, Model*>& newModels, const std::array<std::vector<float>, 3>& vertexData)
{
    // Make sure a model was already created.
    if (model == nullptr) {
        model = new Model("model_" + fs::path(filename).stem().string());
        newModels[model->name] = model;
    }

    // Make sure a mesh was already created.
    if (model->meshes.empty())
        model->meshes.emplace_back("mesh_" + fs::path(filename).stem().string(), *model);

    // Let the current mesh parse its vertices.
    fileContents.seekg(fileContents.tellg() - (std::streamoff)line.size());
    ParseObjMeshVertices(&model->meshes.back(), vertexData, ParseObjMeshIndices(fileContents));
}

std::array<std::vector<uint32_t>, 3> WavefrontParser::ParseObjMeshIndices(std::stringstream& fileContents)
{
    // Holds all vertex data as indices to the vertexData array.
    // Format: indices[0] = pos, indices[1] = uvs, indices[3] = normals.
    std::array<std::vector<uint32_t>, 3> vertexIndices;

    // Read file line by line to create vertex data.
    std::string line;
    while (std::getline(fileContents, line)) 
    {
        line += ' ';
        switch (line[0])
        {
            // Parse object indices.
            case 'f':
                ParseObjTriangle(line, vertexIndices);
            continue;
            // Skip comments, smooth lighting statements and empty lines.
            case '#':
            case 's':
            case ' ':
            case '\n':
            case '\0':
                continue;
            // Stop whenever another symbol is encountered.
            default:
                fileContents.seekg(fileContents.tellg() - (std::streamoff)line.size());
            break;
        }
        break;
    }

    return vertexIndices;
}

void WavefrontParser::ParseObjMeshVertices(Mesh* mesh, const std::array<std::vector<float>, 3>& vertexData, const std::array<std::vector<uint32_t>, 3>& vertexIndices)
{
    // Get the first index.
    uint32_t startIndex = 0;
    if (!mesh->indices.empty())
        startIndex = mesh->indices[mesh->indices.size()-1] + 1;
    
    // Add all parsed data to the vertices array.
    for (uint32_t i = 0; i < vertexIndices[0].size(); i++)
    {
        // Get the current vertex's position.
        Vector3 curPos = Vector3(vertexData[0][vertexIndices[0][i] * (size_t)3], vertexData[0][vertexIndices[0][i] * 3+1], vertexData[0][vertexIndices[0][i] * 3+2]);

        // Get the current vertex's texture coordinates.
        Vector2 curUv;
        if (!vertexData[1].empty()) curUv = Vector2(vertexData[1][vertexIndices[1][i] * (size_t)2], vertexData[1][vertexIndices[1][i] * 2+1]);

        // Get the current vertex's normal.
        Vector3 curNormal;
        if (!vertexData[2].empty()) curNormal = Vector3(vertexData[2][vertexIndices[2][i] * (size_t)3], vertexData[2][vertexIndices[2][i] * 3+1], vertexData[2][vertexIndices[2][i] * 3+2]);

        // Get the current face's tangent and bitangent.
        static Vector3 curTangent, curBitangent;
        if (i % 3 == 0)
        {
            const Vector3 edge1    = Vector3(vertexData[0][vertexIndices[0][i+1] * (size_t)3], vertexData[0][vertexIndices[0][i+1] * 3+1], vertexData[0][vertexIndices[0][i+1] * 3+2]) - curPos;
            const Vector3 edge2    = Vector3(vertexData[0][vertexIndices[0][i+2] * (size_t)3], vertexData[0][vertexIndices[0][i+2] * 3+1], vertexData[0][vertexIndices[0][i+2] * 3+2]) - curPos;
            const Vector2 deltaUv1 = Vector2(vertexData[1][vertexIndices[1][i+1] * (size_t)2], vertexData[1][vertexIndices[1][i+1] * 2+1]) - curUv;
            const Vector2 deltaUv2 = Vector2(vertexData[1][vertexIndices[1][i+2] * (size_t)2], vertexData[1][vertexIndices[1][i+2] * 2+1]) - curUv;

            const float f = 1.0f / (deltaUv1.x * deltaUv2.y - deltaUv2.x * deltaUv1.y);

            curTangent.x = f * (deltaUv2.y * edge1.x - deltaUv1.y * edge2.x);
            curTangent.y = f * (deltaUv2.y * edge1.y - deltaUv1.y * edge2.y);
            curTangent.z = f * (deltaUv2.y * edge1.z - deltaUv1.y * edge2.z);

            curBitangent.x = f * (-deltaUv2.x * edge1.x + deltaUv1.x * edge2.x);
            curBitangent.y = f * (-deltaUv2.x * edge1.y + deltaUv1.x * edge2.y);
            curBitangent.z = f * (-deltaUv2.x * edge1.z + deltaUv1.x * edge2.z);
        }

        // Create a new vertex with the computed data.
        // mesh->vertices.push_back(TangentVertex{ curPos, curUv, curNormal, curTangent, curBitangent });
        mesh->vertices.push_back(Vertex{ curPos, curUv, curNormal });
        mesh->indices .push_back(startIndex + i);
    }

    // TODO: Use the following code to prevent vertex duplication for systems with low GPU memory.
    /*
    // Add all parsed data to the vertices array.
    std::unordered_map<TangentVertex, uint32_t> uniqueVertices{};
    for (uint32_t i = 0; i < vertexIndices[0].size(); i++)
    {
        // Same code as above for position, uv, normal, tangent, bitangent.

        // Create and save the current vertex.
        TangentVertex curVertex = { curPos, curUv, curNormal, curTangent, curBitangent };
        if (uniqueVertices.count(curVertex) <= 0) {
            uniqueVertices[curVertex] = startIndex + i;
            vertices.push_back(curVertex);
        }
        indices.push_back(startIndex + i);
    }
    */
}
#pragma endregion
