#pragma once
#include <webgpu/webgpu_cpp.h>
#include <string>
#include <memory>
#include "GrassSettings.h"
#include "Camera.h"

namespace grass
{
    class Renderer
    {
    public:
        Renderer(std::shared_ptr<wgpu::Device> device, std::shared_ptr<wgpu::Queue> queue,
                 wgpu::TextureFormat surfaceFormat);
        ~Renderer() = default;
        void init(const std::string& meshFilePath, const wgpu::Buffer& positionsBuffer, const Camera& camera);
        void draw(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView,
                  const GrassSettings& grassSettings);
        void updateUniforms(const Camera& camera);

    private:
        void createVertexBuffer(const std::string& meshFilePath);
        void createUniformBuffer(const Camera& camera);
        void createDepthTextureView();
        void initRenderPipeline(const wgpu::Buffer& positionsBuffer);

        std::shared_ptr<wgpu::Device> device;
        std::shared_ptr<wgpu::Queue> queue;
        wgpu::TextureFormat surfaceFormat;

        wgpu::Buffer vertexBuffer;
        size_t vertexCount = 0;
        wgpu::Buffer uniformBuffer;
        wgpu::RenderPipeline grassPipeline;
        wgpu::BindGroup grassBindGroup;
        wgpu::TextureView depthView;
    };
} // grass
