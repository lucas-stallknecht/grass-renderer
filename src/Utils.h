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

    wgpu::ShaderModule getShaderModule(const wgpu::Device& device, const std::string& shaderPath,
                                       const std::string& moduleLabel, bool includeCommonStructs = true);
    void parseShaderFile(const std::string& filePath, std::string& sourceCode);
    bool loadMesh(const std::string& filePath, std::vector<VertexData>& verticesData);
    wgpu::Texture loadTexture(const std::string& path,
        const wgpu::Device& device, const wgpu::Queue& queue);
}
