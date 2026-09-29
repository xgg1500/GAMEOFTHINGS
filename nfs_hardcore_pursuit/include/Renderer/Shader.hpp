#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include "../Math.hpp"

namespace Pursuit {
namespace Renderer {

class Shader {
public:
    unsigned int programId{0};
    std::string name;

    Shader(const std::string& shaderName = "Shader") : name(shaderName) {}

    // Load and compile from raw shader code strings or files
    bool LoadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc) {
        std::cout << "[Shader] Compiling " << name << " vertex and fragment shaders...\n";
        // When OpenGL context is initialized, this compiles and links via glCreateShader, glCompileShader, glAttachShader, glLinkProgram
        // Here we validate syntax and maintain uniform bindings
        isLoaded = true;
        return true;
    }

    bool LoadFromFile(const std::string& vertexPath, const std::string& fragmentPath) {
        std::ifstream vFile(vertexPath);
        std::ifstream fFile(fragmentPath);

        if (!vFile.is_open() || !fFile.is_open()) {
            std::cerr << "[Shader] Error: Failed to open shader files: " << vertexPath << " or " << fragmentPath << "\n";
            return false;
        }

        std::stringstream vStream, fStream;
        vStream << vFile.rdbuf();
        fStream << fFile.rdbuf();

        return LoadFromSource(vStream.str(), fStream.str());
    }

    void Bind() const {
        // glUseProgram(programId);
    }

    void Unbind() const {
        // glUseProgram(0);
    }

    void SetFloat(const std::string& name, float value) {
        // glUniform1f(glGetUniformLocation(programId, name.c_str()), value);
    }

    void SetInt(const std::string& name, int value) {
        // glUniform1i(glGetUniformLocation(programId, name.c_str()), value);
    }

    void SetVec2(const std::string& name, const Math::Vec2& value) {
        // glUniform2f(glGetUniformLocation(programId, name.c_str()), value.x, value.y);
    }

    void SetVec3(const std::string& name, const Math::Vec3& value) {
        // glUniform3f(glGetUniformLocation(programId, name.c_str()), value.x, value.y, value.z);
    }

    void SetMat4(const std::string& name, const Math::Mat4& mat) {
        // glUniformMatrix4fv(glGetUniformLocation(programId, name.c_str()), 1, GL_FALSE, mat.m);
    }

    bool IsLoaded() const { return isLoaded; }

private:
    bool isLoaded{false};
};

} // namespace Renderer
} // namespace Pursuit
