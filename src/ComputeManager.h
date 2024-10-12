#pragma once

#include <webgpu/webgpu_cpp.h>
#include "GrassSettings.h"


namespace grass {

class ComputeManager {

public:
    ComputeManager(std::shared_ptr<wgpu::Device> device, std::shared_ptr<wgpu::Queue> queue);
    wgpu::Buffer init(const GrassSettings& grassSettings);
    void compute(const GrassSettings& grassSettings);

private:
    void createBuffers(const GrassSettings& grassSettings);
    void initComputPipeline();

    std::shared_ptr<wgpu::Device> device;
    std::shared_ptr<wgpu::Queue> queue;

    wgpu::ComputePipeline computePipeline;
    wgpu::Buffer bladesPosBuffer;
    wgpu::Buffer grassSettingsUniformBuffer;
    wgpu::BindGroup computeBindGroup;
};

} // grass
