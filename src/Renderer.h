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
        const uint32_t MULTI_SAMPLE_COUNT = 1;

    public:
        Renderer(std::shared_ptr<GlobalConfig> config, uint16_t width, uint16_t height);
        ~Renderer() = default;
        void init(const wgpu::Buffer& computeBuffer);
        void render(const std::vector<Mesh>& scene, const Camera& camera, float time);
        void updateBladeUniforms(const BladeStaticUniformData& uniform);

    private:
        void initGlobalResources();
        void initBladeResources();
        void createDepthTextureView();
        void initSkyPipeline();
        void initGrassPipeline(const wgpu::Buffer& computeBuffer);
        void initPhongPipeline();
        void initSSSPipeline();
        void updateGlobalUniforms(const Camera& camera, float time);
        void drawSky(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView);
        void drawGrass(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView);
        void drawScene(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView, const std::vector<Mesh>& scene);
        void drawGUI(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView);

        std::shared_ptr<GlobalConfig> config;
        GPUContext* ctx = nullptr;
        wgpu::Extent2D size;
        wgpu::Texture multisampleTexture;
        wgpu::TextureView depthView;
        wgpu::Sampler textureSampler;

        wgpu::BindGroup globalBindGroup;
        wgpu::Buffer globalUniformBuffer;

        wgpu::RenderPipeline phongPipeline;

        wgpu::RenderPipeline skyPipeline;
        MeshGeomoetry fullScreenQuad{"../assets/full_screen_quad.obj"};

        wgpu::RenderPipeline grassPipeline;
        wgpu::Buffer bladeUniformBuffer;
        wgpu::BindGroup grassBindGroup;
        wgpu::Texture bladeNormalTexture;
        MeshGeomoetry bladeGeometry{"../assets/grass_blade.obj"};

        wgpu::RenderPipeline sssPipeline;
        wgpu::BindGroup sssBindGroup;
        wgpu::Buffer sssBuffer;
    };
} // grass
