#pragma once

#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>

#include "Context.h"

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
        ComputeManager(std::shared_ptr<Context> ctx);
        wgpu::Buffer init();
        void updateMovSettingsUniorm();
        void generate();
        void computeMovement(float time);

    private:
        void createSharedBuffer();
        wgpu::BindGroupLayout createSharedBindGroup();
        void createUniformBuffers();
        void initGenPipeline(const wgpu::BindGroupLayout& sharedLayout);
        void initMovPipeline(const wgpu::BindGroupLayout& sharedLayout);

        std::shared_ptr<Context> ctx;

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
