#pragma once

#include <webgpu/webgpu_cpp.h>
#include <string>
#include <glm/glm.hpp>
#include <tiny_obj_loader.h>
#include <stb_image.h>
#include <iostream>
#include <fstream>
#include <sstream>

#define COMMON_STRUCTS_PATH "../shaders/common_structs.wgsl"


namespace grass
{
    struct VertexData
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
    };


    inline void parseShaderFile(const std::string& filePath, std::string& sourceCode)
    {
        std::ifstream shaderFile;
        // ensure ifstream objects can throw exceptions:
        shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
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
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ : " << filePath << std::endl;
        }
    }


    inline wgpu::ShaderModule getShaderModule(const wgpu::Device& device, const std::string& shaderPath,
                                              const std::string& moduleLabel,
                                              bool includeCommonStructs = true)
    {
        std::string shaderCode;
        parseShaderFile(shaderPath, shaderCode);

        if (includeCommonStructs)
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


    inline bool loadVertexData(const std::string& filePath, std::vector<VertexData>& verticesData)
    {
        tinyobj::ObjReaderConfig readerConfig;
        readerConfig.mtl_search_path = "./";

        tinyobj::ObjReader reader;

        if (!reader.ParseFromFile(filePath, readerConfig))
        {
            if (!reader.Error().empty())
            {
                std::cerr << "TinyObjReader: " << reader.Error();
            }
            return false;
        }

        if (!reader.Warning().empty())
        {
            std::cout << "TinyObjReader: " << reader.Warning();
        }

        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();

        const auto& shape = shapes[0]; // look at the first shape only

        size_t i = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
        {
            const auto fv = size_t(shape.mesh.num_face_vertices[f]);

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++)
            {
                // access to vertex
                VertexData vData = {};
                tinyobj::index_t idx = shape.mesh.indices[i + v];
                vData.position = {
                    attrib.vertices[3 * size_t(idx.vertex_index) + 0],
                    attrib.vertices[3 * size_t(idx.vertex_index) + 1],
                    attrib.vertices[3 * size_t(idx.vertex_index) + 2]
                };

                // Check if `normal_index` is zero or positive. negative = no normal data
                if (idx.normal_index >= 0)
                {
                    vData.normal = {
                        attrib.normals[3 * size_t(idx.normal_index) + 0],
                        attrib.normals[3 * size_t(idx.normal_index) + 1],
                        attrib.normals[3 * size_t(idx.normal_index) + 2]
                    };
                }

                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                if (idx.texcoord_index >= 0)
                {
                    vData.texCoord = {
                        attrib.texcoords[2 * size_t(idx.texcoord_index) + 0],
                        1.0 - attrib.texcoords[2 * size_t(idx.texcoord_index) + 1],
                    };
                }
                verticesData.push_back(vData);
            }
            i += fv;
        }

        return true;
    }


    inline wgpu::Texture loadTexture(const std::string& path)
    {
        int width, height, channels;
        unsigned char* pixelData = stbi_load(path.c_str(), &width, &height, &channels, 4);
        if (nullptr == pixelData) return nullptr;

        const wgpu::Device device = GPUContext::getInstance()->getDevice();
        const wgpu::Queue queue = GPUContext::getInstance()->getQueue();

        wgpu::Extent3D textureSize = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};

        wgpu::TextureDescriptor textureDesc;
        textureDesc.dimension = wgpu::TextureDimension::e2D;
        textureDesc.format = wgpu::TextureFormat::RGBA8Unorm;
        // by convention for bmp, png and jpg file. Be careful with other formats.
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = 1;
        textureDesc.size = textureSize;
        textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
        textureDesc.viewFormatCount = 0;
        textureDesc.viewFormats = nullptr;
        wgpu::Texture texture = device.CreateTexture(&textureDesc);

        wgpu::ImageCopyTexture dest = {
            .texture = texture,
            .origin = {0, 0, 0},
        };
        wgpu::TextureDataLayout source = {
            .bytesPerRow = 4 * textureSize.width,
            .rowsPerImage = textureSize.height
        };

        queue.WriteTexture(&dest, pixelData, 4 * width * height, &source, &textureSize);

        stbi_image_free(pixelData);

        return texture;
    }
}
