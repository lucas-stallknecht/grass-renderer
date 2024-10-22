#include "Utils.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <iostream>
#include <fstream>
#include <sstream>

#define COMMON_STRUCTS_PATH "../shaders/common_structs.wgsl"

wgpu::ShaderModule grass::getShaderModule(const wgpu::Device& device, const std::string& shaderPath, const std::string& moduleLabel,
    bool includeCommonStructs)
{
    std::string shaderCode;
    parseShaderFile(shaderPath, shaderCode);

    if(includeCommonStructs)
    {
        std::string structsCode;
        parseShaderFile(COMMON_STRUCTS_PATH, structsCode);
        shaderCode.insert(0, structsCode);
    }

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


bool grass::loadMesh(const std::string& filePath, std::vector<VertexData>& verticesData)
{
    tinyobj::ObjReaderConfig readerConfig;
    readerConfig.mtl_search_path = "./";

    tinyobj::ObjReader reader;

    if(!reader.ParseFromFile(filePath, readerConfig))
    {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        return false;
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();

    const auto& shape = shapes[0]; // look at the first shape only

    verticesData.resize(shape.mesh.indices.size());
    for (size_t i = 0; i < shape.mesh.indices.size(); ++i) {
        const tinyobj::index_t& idx = shape.mesh.indices[i];

        verticesData[i].position = {
            attrib.vertices[3 * idx.vertex_index + 0],
            attrib.vertices[3 * idx.vertex_index + 1],
            attrib.vertices[3 * idx.vertex_index + 2]
        };

        verticesData[i].normal = {
            attrib.normals[3 * idx.normal_index + 0],
            attrib.normals[3 * idx.normal_index + 1],
            attrib.normals[3 * idx.normal_index + 2]
        };

        verticesData[i].texCoord = {
            attrib.texcoords[2 * idx.vertex_index + 0],
            attrib.texcoords[2 * idx.vertex_index + 1],
        };
    }
    return true;
}


