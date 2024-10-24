#pragma once

#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>
#include "GrassSettings.h"


namespace grass
{
    struct Blade
    {
        glm::vec3 position;
        float_t size;
        glm::vec2 uv;
        float_t angle;
        float_t padding;
    };

    class ComputeManager
    {
    public:
        ComputeManager(std::shared_ptr<wgpu::Device> device, std::shared_ptr<wgpu::Queue> queue);
        wgpu::Buffer init(const GrassGenerationSettings& grassSettings);
        void compute(const GrassGenerationSettings& grassSettings);

    private:
        void createBuffers(const GrassGenerationSettings& grassSettings);
        void initComputPipeline(const GrassGenerationSettings& grassSettings);

        std::shared_ptr<wgpu::Device> device;
        std::shared_ptr<wgpu::Queue> queue;

        wgpu::ComputePipeline computePipeline;
        wgpu::Buffer computeBuffer;
        wgpu::Buffer grassSettingsUniformBuffer;
        wgpu::BindGroup computeBindGroup;
    };
} // grass
