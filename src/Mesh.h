#pragma once
#include <webgpu/webgpu_cpp.h>

namespace grass {

class Mesh {
public:
    Mesh(const std::string& meshFilePath, const wgpu::RenderPipeline& renderPipeline);
    void draw(const wgpu::RenderPassEncoder& pass, uint32_t instanceCount);

private:
    void loadGeometry(const std::string& meshFilePath);

    wgpu::Buffer vertexBuffer;
    size_t vertexCount = 0;

    wgpu::RenderPipeline pipeline;
};

} // grass
