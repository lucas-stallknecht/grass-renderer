#pragma once

#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>
#include "GrassSettings.h"


namespace grass
{
    struct Blade
    {
        glm::vec3 c0;
        float idHash;
        glm::vec2 uv;
        float height;
        float padding;
        glm::vec4 c1;
        glm::vec4 c2;
        glm::vec4 facingDirection;
    };

    class ComputeManager
    {
    public:
        ComputeManager(std::shared_ptr<wgpu::Device> device, std::shared_ptr<wgpu::Queue> queue);
        wgpu::Buffer init(const GrassGenerationSettings& genSettings, const GrassMovUniformData& movSettings);
        void updateMovSettingsUniorm(const GrassMovUniformData& movSettings);
        void generate(const GrassGenerationSettings& genSettings);
        void computeMovement(const GrassGenerationSettings& genSettings, float time);

    private:
        void createSharedBuffer(const GrassGenerationSettings& genSettings);
        wgpu::BindGroupLayout createSharedBindGroup();
        void createUniformBuffers(const GrassGenerationSettings& genSettings, const GrassMovUniformData& movSettings);
        void initGenPipeline(const GrassGenerationSettings& genSettings, const wgpu::BindGroupLayout& sharedLayout);
        void initMovPipeline(const GrassMovUniformData& movSettings, const wgpu::BindGroupLayout& sharedLayout);

        std::shared_ptr<wgpu::Device> device;
        std::shared_ptr<wgpu::Queue> queue;

        wgpu::Buffer computeBuffer;
        wgpu::BindGroup sharedBindGroup;

        wgpu::ComputePipeline genPipeline;
        wgpu::Buffer genSettingsUniformBuffer;
        wgpu::BindGroup genBindGroup;

        wgpu::ComputePipeline movPipeline;
        wgpu::Buffer movSettingsUniformBuffer;
        wgpu::Buffer movDynamicUniformBuffer;
        wgpu::BindGroup movBindGroup;
    };
} // grass
