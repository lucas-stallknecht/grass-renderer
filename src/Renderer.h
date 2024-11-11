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
        const bool USE_MULTI_SAMPLE = true;
        const uint32_t MULTI_SAMPLE_COUNT = USE_MULTI_SAMPLE ? 4 : 1;

    public:
        Renderer(std::shared_ptr<GlobalConfig> config, uint16_t width, uint16_t height);
        ~Renderer() = default;
        void init(const wgpu::Buffer& computeBuffer);
        void render(const std::vector<Mesh>& scene, const Camera& camera, float time, uint32_t frameNumber);
        void toggleGUI();
        void updateBladeUniforms();
        void updateShadowUniforms();

    private:
        void initGlobalResources();
        void initBladeResources();
        void initShadowResources();
        void createDepthTextureView();
        void initSkyPipeline();
        void initGrassPipeline(const wgpu::Buffer& computeBuffer);
        void initPhongPipeline();
        void initShadowPipeline();
        void updateGlobalUniforms(const Camera& camera, float time, uint32_t frameNumber);
        void drawSky(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView);
        void drawGrass(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView);
        void drawScene(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView, const std::vector<Mesh>& scene);
        void drawGUI(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView);

        std::shared_ptr<GlobalConfig> config;
        bool showGui = true;
        GPUContext* ctx = nullptr;
        wgpu::Extent2D size;
        wgpu::TextureView depthView;
        wgpu::TextureView multisampleView;

        wgpu::BindGroup globalBindGroup;
        wgpu::Buffer globalUniformBuffer;
        wgpu::Sampler globalTextureSampler;

        wgpu::RenderPipeline phongPipeline;

        wgpu::RenderPipeline skyPipeline;
        MeshGeomoetry fullScreenQuad{"../assets/full_screen_quad.obj"};

        wgpu::RenderPipeline grassPipeline;
        wgpu::Buffer bladeUniformBuffer;
        wgpu::BindGroup bladeUniformBindGroup;
        wgpu::Texture bladeNormalTexture;
        MeshGeomoetry bladeGeometry{"../assets/grass_blade.obj"};

        // TODO ScreenSpaceShadow could be done by compute shader
        wgpu::RenderPipeline shadowPipeline;
        wgpu::BindGroup shadowUniformBindGroup;
        wgpu::Buffer shadowUniformBuffer;
        wgpu::Texture shadowTexture;
    };
} // grass
