#pragma once

#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>

#include "GPUContext.h"
#include "GlobalConfig.h"

namespace grass
{
    struct Blade
    {
        glm::vec3 c0;
        float idHash;
        glm::vec2 uv;
        float height;
        float relativeHeight;
        glm::vec4 c1;
        glm::vec4 c2;
        glm::vec3 facingDirection;
        float collisionStrength;
    };

    class ComputeManager
    {
    public:
        explicit ComputeManager(std::shared_ptr<GlobalConfig> config);
        wgpu::Buffer init();
        void updateMovSettingsUniorm();
        void generate();
        void computeMovement(float time);

    private:
        bool createSharedBuffer();
        bool createSharedBindGroup();
        bool createUniformBuffers();
        bool initGenPipeline();
        bool initMovPipeline();

        std::shared_ptr<GlobalConfig> config;
        GPUContext* ctx = nullptr;

        wgpu::Buffer computeBuffer;
        wgpu::BindGroupLayout sharedLayout;
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
