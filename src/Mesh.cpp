#include "Mesh.h"

#include "GPUContext.h"
#include "layouts.h"
#include "Utils.h"

namespace grass {

    MeshGeomoetry::MeshGeomoetry(const std::string& meshFilePath)
    {
        createVertexBuffer(meshFilePath);
    }


    void MeshGeomoetry::createVertexBuffer(const std::string& meshFilePath)
    {
        std::vector<VertexData> verticesData;
        if (!loadVertexData(meshFilePath, verticesData))
        {
            std::cerr << "Could not load geometry!" << std::endl;
        }

        vertexCount = verticesData.size();
        std::string label = "Vertex buffer :" + meshFilePath;
        wgpu::BufferDescriptor bufferDesc{
            .label = label.c_str(),
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
            .size = verticesData.size() * sizeof(VertexData),
            .mappedAtCreation = false,
        };
        vertexBuffer = GPUContext::getInstance()->getDevice().CreateBuffer(&bufferDesc);
        GPUContext::getInstance()->getQueue().WriteBuffer(vertexBuffer, 0, verticesData.data(), bufferDesc.size);
    }

    void MeshGeomoetry::draw(const wgpu::RenderPassEncoder& pass, uint32_t instanceCount)
    {
        pass.SetVertexBuffer(0, vertexBuffer, 0, vertexBuffer.GetSize());
        pass.Draw(vertexCount, instanceCount, 0, 0);
    }

   Mesh::Mesh(MeshGeomoetry geometry, PhongMaterial material) : geometry(std::move(geometry)), material(std::move(material))
   {
        createBuffer();
        createBindGroup();
   }

    void Mesh::createBuffer()
    {
        wgpu::BufferDescriptor bufferDesc = {
            .label = "Model uniform buffer",
            .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
            .size = sizeof(glm::mat4),
            .mappedAtCreation = false
        };
        modelBuffer = GPUContext::getInstance()->getDevice().CreateBuffer(&bufferDesc);
    }


    void Mesh::createBindGroup()
    {
        wgpu::BindGroupEntry entry[1] = {
            {
                .binding = 0,
                .buffer = modelBuffer,
                .offset = 0,
                .size = modelBuffer.GetSize()
            },
        };
        wgpu::BindGroupDescriptor bindGroupDesc = {
            .label = "Model bind group",
            .layout = GPUContext::getInstance()->getDevice().CreateBindGroupLayout(&modelBindGroupLayoutDesc),
            .entryCount = 1,
            .entries = &entry[0]
        };
        bindGroup = GPUContext::getInstance()->getDevice().CreateBindGroup(&bindGroupDesc);
    }


    void Mesh::draw(const wgpu::RenderPassEncoder& pass, uint32_t instanceCount)
    {
        GPUContext::getInstance()->getQueue().WriteBuffer(
            modelBuffer,
            0,
            &model,
            modelBuffer.GetSize()
        );
        geometry.draw(pass, instanceCount);
    }





} // grass