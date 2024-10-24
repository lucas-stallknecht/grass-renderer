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
                  const GrassGenerationSettings& grassSettings);
        void updateUniforms(const Camera& camera, GrassVertexSettingsUniforms& settingsUniforms, float_t time);

    private:
        void createVertexBuffer(const std::string& meshFilePath);
        void createUniformBuffers(const Camera& camera);
        void createBladeTextures();
        void createDepthTextureView();
        void initRenderPipeline(const wgpu::Buffer& positionsBuffer);

        std::shared_ptr<wgpu::Device> device;
        std::shared_ptr<wgpu::Queue> queue;
        wgpu::TextureFormat surfaceFormat;

        wgpu::Buffer vertexBuffer;
        size_t vertexCount = 0;
        wgpu::Buffer camUniformBuffer;
        wgpu::Buffer settingsUniformBuffer;
        wgpu::RenderPipeline grassPipeline;
        wgpu::BindGroup globalBindGroup;
        wgpu::BindGroup storageBindGroup;
        wgpu::TextureView depthView;
        wgpu::Texture normalTexture;
        wgpu::Sampler textureSampler;
    };
} // grass
