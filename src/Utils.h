#pragma once

#include <webgpu/webgpu_cpp.h>
#include <string>
#include <glm/glm.hpp>

namespace grass
{
    struct VertexData
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
    };

    wgpu::ShaderModule getShaderModule(wgpu::Device& device, std::string shaderPath, std::string moduleLabel);
    void parseShaderFile(const std::string& filePath, std::string& sourceCode);
    bool loadMesh(const std::string& filePath, std::vector<VertexData>& verticesData);
}
