#pragma once
#include <webgpu/webgpu_cpp.h>
#include <string>
#include <memory>
#include "Camera.h"
#include "GPUContext.h"
#include "GlobalConfig.h"
#include "Mesh.h"

namespace grass
{
    class Renderer
    {
    public:
        Renderer(std::shared_ptr<GlobalConfig> config, uint16_t width, uint16_t height);
        ~Renderer() = default;
        void init(const std::string& meshFilePath, const wgpu::Buffer& positionsBuffer, const Camera& camera);
        void draw(Mesh& mesh);
        void updateDynamicUniforms(const Camera& camera, float time);
        void updateStaticUniforms(const BladeStaticUniformData& uniform);

    private:
        void createUniformBuffers(const Camera& camera);
        void createBladeTextures();
        void createDepthTextureView();
        void initRenderPipeline(const wgpu::Buffer& computeBuffer);
        void drawScene(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView, Mesh& mesh);
        void drawGUI(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView);

        std::shared_ptr<GlobalConfig> config;
        GPUContext* ctx = nullptr;
        wgpu::Extent2D size;
        wgpu::TextureView depthView;
        wgpu::Texture normalTexture;
        wgpu::Sampler textureSampler;

        wgpu::Buffer camUniformBuffer;
        wgpu::Buffer bladeStaticUniformBuffer;
        wgpu::Buffer bladeDynamicUniformBuffer;

        wgpu::RenderPipeline grassPipeline;
        wgpu::BindGroup globalBindGroup;
        wgpu::BindGroup storageBindGroup;
    };
} // grass
