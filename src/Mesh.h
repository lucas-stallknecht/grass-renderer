#pragma once
#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>

#include "PhongMaterial.h"

namespace grass {

class MeshGeomoetry {
public:
    explicit MeshGeomoetry(const std::string& meshFilePath);
    void draw(const wgpu::RenderPassEncoder& pass, uint32_t instanceCount);

private:
    void createVertexBuffer(const std::string& meshFilePath);

    wgpu::Buffer vertexBuffer;
    size_t vertexCount = 0;
};


class Mesh
{
public:
    Mesh(MeshGeomoetry geometry, PhongMaterial material);
    void draw(const wgpu::RenderPassEncoder& pass, uint32_t instanceCount);

    MeshGeomoetry geometry;
    PhongMaterial material;
    glm::mat4 model = glm::mat4(1.0f);

    wgpu::Buffer modelBuffer;
    wgpu::BindGroup bindGroup;

private:
    void createBindGroup();
    void createBuffer();
};

} // grass
