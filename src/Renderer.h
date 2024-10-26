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
        uint16_t width, uint16_t height, wgpu::TextureFormat surfaceFormat);
        ~Renderer() = default;
        void init(const std::string& meshFilePath, const wgpu::Buffer& positionsBuffer, const Camera& camera);
        void draw(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView,
                  const GrassGenerationSettings& grassSettings);
        void updateDynamicUniforms(const Camera& camera, float time);
        void updateStaticUniforms(const BladeStaticUniformData& uniform);

    private:
        void createVertexBuffer(const std::string& meshFilePath);
        void createUniformBuffers(const Camera& camera);
        void createBladeTextures();
        void createDepthTextureView();
        void initRenderPipeline(const wgpu::Buffer& positionsBuffer);

        std::shared_ptr<wgpu::Device> device;
        std::shared_ptr<wgpu::Queue> queue;
        wgpu::TextureFormat surfaceFormat;
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
