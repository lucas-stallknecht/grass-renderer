#include "Mesh.h"

#include "GPUContext.h"
#include "Utils.h"

namespace grass {

    Mesh::Mesh(const std::string& meshFilePath, const wgpu::RenderPipeline& renderPipeline)
    {
        loadGeometry(meshFilePath);
        pipeline = renderPipeline;
    }


    void Mesh::loadGeometry(const std::string& meshFilePath)
    {
        std::vector<VertexData> verticesData;
        if (!loadMesh(meshFilePath, verticesData))
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

    void Mesh::draw(const wgpu::RenderPassEncoder& pass, uint32_t instanceCount)
    {
        pass.SetVertexBuffer(0, vertexBuffer, 0, vertexBuffer.GetSize());
        pass.Draw(vertexCount, instanceCount, 0, 0);
    }


} // grass