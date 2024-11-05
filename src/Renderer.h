#pragma once
#include <webgpu/webgpu_cpp.h>
#include <string>
#include <memory>
#include "Camera.h"
#include "Context.h"

namespace grass
{
    class Renderer
    {
    public:
        Renderer(std::shared_ptr<Context> ctx, uint16_t width, uint16_t height);
        ~Renderer() = default;
        void init(const std::string& meshFilePath, const wgpu::Buffer& positionsBuffer, const Camera& camera);
        void draw(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView, size_t nBlades);
        void updateDynamicUniforms(const Camera& camera, float time);
        void updateStaticUniforms(const BladeStaticUniformData& uniform);

    private:
        void createVertexBuffer(const std::string& meshFilePath);
        void createUniformBuffers(const Camera& camera);
        void createBladeTextures();
        void createDepthTextureView();
        void initRenderPipeline(const wgpu::Buffer& computeBuffer);

        std::shared_ptr<Context> ctx;
        wgpu::Extent2D size;
        wgpu::TextureView depthView;
        wgpu::Texture normalTexture;
        wgpu::Sampler textureSampler;

        wgpu::Buffer vertexBuffer;
        size_t vertexCount = 0;
        wgpu::Buffer camUniformBuffer;
        wgpu::Buffer bladeStaticUniformBuffer;
        wgpu::Buffer bladeDynamicUniformBuffer;

        wgpu::RenderPipeline grassPipeline;
        wgpu::BindGroup globalBindGroup;
        wgpu::BindGroup storageBindGroup;


    };
} // grass
