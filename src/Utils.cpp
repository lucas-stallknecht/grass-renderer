#include "Utils.h"

#include <iostream>
#include <fstream>
#include <sstream>

wgpu::ShaderModule grass::getShaderModule(wgpu::Device& device, std::string shaderPath, std::string moduleLabel)
{
    std::string shaderCode;
    parseShaderFile(shaderPath, shaderCode);

    wgpu::ShaderModuleWGSLDescriptor wgslDesc;
    wgslDesc.code = shaderCode.c_str();

    wgpu::ShaderModuleDescriptor shaderModuleDesc{
        .nextInChain = &wgslDesc,
        .label = moduleLabel.c_str()
    };

    return device.CreateShaderModule(&shaderModuleDesc);

}

void grass::parseShaderFile(const std::string &filePath,  std::string& sourceCode) {
    std::ifstream shaderFile;
    // ensure ifstream objects can throw exceptions:
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        // open files
        shaderFile.open(filePath);
        std::stringstream shaderStream;
        // read file's buffer contents into streams
        shaderStream << shaderFile.rdbuf();
        // close file handlers
        shaderFile.close();
        // convert stream into string
        sourceCode = shaderStream.str();
    }
    catch (std::ifstream::failure e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ : " << filePath << std::endl;
    }
}

