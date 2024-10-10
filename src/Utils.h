#pragma once

#include <webgpu/webgpu_cpp.h>
#include <string>

namespace grass
{
    wgpu::ShaderModule getShaderModule(wgpu::Device& device, std::string shaderPath, std::string moduleLabel);

    void parseShaderFile(const std::string& filePath, std::string& sourceCode);
}
