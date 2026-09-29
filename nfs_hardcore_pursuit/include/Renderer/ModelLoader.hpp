#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include "../Math.hpp"

namespace Pursuit {
namespace Renderer {

struct Vertex {
    Math::Vec3 position;
    Math::Vec3 normal;
    Math::Vec2 texCoords;
    Math::Vec3 tangent;
    Math::Vec3 bitangent;
};

struct Mesh {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    unsigned int vao{0};
    unsigned int vbo{0};
    unsigned int ebo{0};

    // Prepare GPU Buffers when OpenGL context is active
    void SetupMesh() {
        // glGenVertexArrays(1, &vao);
        // glGenBuffers(1, &vbo);
        // glGenBuffers(1, &ebo);
        // glBindVertexArray(vao);
        // glBindBuffer(GL_ARRAY_BUFFER, vbo);
        // glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        // glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    }
};

class ModelLoader {
public:
    // Load 3D Wavefront .OBJ Car Model
    static bool LoadOBJ(const std::string& filePath, Mesh& outMesh) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "[ModelLoader] Error: Could not open 3D model file: " << filePath << "\n";
            return false;
        }

        std::cout << "[ModelLoader] Loading external 3D car model: " << filePath << "...\n";

        std::vector<Math::Vec3> tempPositions;
        std::vector<Math::Vec2> tempTexCoords;
        std::vector<Math::Vec3> tempNormals;

        std::string line;
        while (std::getline(file, line)) {
            if (line.substr(0, 2) == "v ") {
                std::istringstream s(line.substr(2));
                Math::Vec3 v;
                s >> v.x >> v.y >> v.z;
                tempPositions.push_back(v);
            } else if (line.substr(0, 3) == "vt ") {
                std::istringstream s(line.substr(3));
                Math::Vec2 vt;
                s >> vt.x >> vt.y;
                tempTexCoords.push_back(vt);
            } else if (line.substr(0, 3) == "vn ") {
                std::istringstream s(line.substr(3));
                Math::Vec3 vn;
                s >> vn.x >> vn.y >> vn.z;
                tempNormals.push_back(vn);
            } else if (line.substr(0, 2) == "f ") {
                std::istringstream s(line.substr(2));
                std::string vertexStr;
                while (s >> vertexStr) {
                    std::replace(vertexStr.begin(), vertexStr.end(), '/', ' ');
                    std::istringstream elem(vertexStr);
                    int pIdx = 0, tIdx = 0, nIdx = 0;
                    elem >> pIdx >> tIdx >> nIdx;

                    Vertex vert;
                    if (pIdx > 0 && pIdx <= (int)tempPositions.size()) vert.position = tempPositions[pIdx - 1];
                    if (tIdx > 0 && tIdx <= (int)tempTexCoords.size()) vert.texCoords = tempTexCoords[tIdx - 1];
                    if (nIdx > 0 && nIdx <= (int)tempNormals.size()) vert.normal = tempNormals[nIdx - 1];

                    outMesh.indices.push_back((unsigned int)outMesh.vertices.size());
                    outMesh.vertices.push_back(vert);
                }
            }
        }

        ComputeTangents(outMesh);
        std::cout << "[ModelLoader] Loaded " << outMesh.vertices.size() << " vertices, " 
                  << outMesh.indices.size() / 3 << " triangles successfully!\n";
        return true;
    }

    // Generate Tangent & Bitangent vectors for normal mapping shader
    static void ComputeTangents(Mesh& mesh) {
        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            if (i + 2 >= mesh.indices.size()) break;
            Vertex& v0 = mesh.vertices[mesh.indices[i]];
            Vertex& v1 = mesh.vertices[mesh.indices[i + 1]];
            Vertex& v2 = mesh.vertices[mesh.indices[i + 2]];

            Math::Vec3 edge1 = v1.position - v0.position;
            Math::Vec3 edge2 = v2.position - v0.position;
            Math::Vec2 deltaUV1 = v1.texCoords - v0.texCoords;
            Math::Vec2 deltaUV2 = v2.texCoords - v0.texCoords;

            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y + 1e-6f);
            Math::Vec3 tangent = (edge1 * deltaUV2.y - edge2 * deltaUV1.y) * f;
            Math::Vec3 bitangent = (edge2 * deltaUV1.x - edge1 * deltaUV2.x) * f;

            v0.tangent = v0.tangent + tangent;
            v1.tangent = v1.tangent + tangent;
            v2.tangent = v2.tangent + tangent;

            v0.bitangent = v0.bitangent + bitangent;
            v1.bitangent = v1.bitangent + bitangent;
            v2.bitangent = v2.bitangent + bitangent;
        }

        for (auto& v : mesh.vertices) {
            v.tangent = v.tangent.Normalized();
            v.bitangent = v.bitangent.Normalized();
        }
    }
};

} // namespace Renderer
} // namespace Pursuit
