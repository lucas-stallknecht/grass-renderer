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
        void init(const wgpu::Buffer& computeBuffer);
        void draw(const std::vector<Mesh>& scene);
        void updateGlobalUniforms(const Camera& camera, float time);
        void updateBladeUniforms(const BladeStaticUniformData& uniform);

    private:
        void initGlobalResources();
        void initBladeResources();
        void createDepthTextureView();
        void initGrassPipeline(const wgpu::Buffer& computeBuffer);
        void initPhongPipeline();
        void drawScene(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView, const std::vector<Mesh>& scene);
        void drawGUI(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView);

        std::shared_ptr<GlobalConfig> config;
        GPUContext* ctx = nullptr;
        wgpu::Extent2D size;
        wgpu::TextureView depthView;
        wgpu::Sampler textureSampler;

        wgpu::BindGroup globalBindGroup;
        wgpu::Buffer globalUniformBuffer;

        wgpu::RenderPipeline phongPipeline;


        wgpu::RenderPipeline grassPipeline;
        wgpu::Buffer bladeUniformBuffer;
        wgpu::BindGroup grassBindGroup;
        wgpu::Texture bladeNormalTexture;
        MeshGeomoetry bladeGeometry{"../assets/grass_blade.obj"};
    };
} // grass
