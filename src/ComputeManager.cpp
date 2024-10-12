#include "ComputeManager.h"

#include <glm/glm.hpp>
#include "Utils.h"

namespace grass
{
    ComputeManager::ComputeManager(std::shared_ptr<wgpu::Device> device, std::shared_ptr<wgpu::Queue> queue)
        : device(std::move(device)), queue(std::move(queue))
    {
    }

    wgpu::Buffer ComputeManager::init(const GrassSettings& grassSettings)
    {
        createBuffers(grassSettings);
        initComputPipeline();

        return bladesPosBuffer;
    }


    void ComputeManager::createBuffers(const GrassSettings& grassSettings)
    {
        wgpu::BufferDescriptor posBufferDesc = {
            .label = "Positions storage buffer",
            .usage = wgpu::BufferUsage::Storage,
            .size = sizeof(glm::vec4) * grassSettings.totalBlades,
            .mappedAtCreation = false
        };

        bladesPosBuffer = device->CreateBuffer(&posBufferDesc);

        wgpu::BufferDescriptor settingsBufferDesc = {
            .label = "Grass settings uniform buffer",
            .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
            .size = sizeof(glm::vec3),
            .mappedAtCreation = false
        };

        grassSettingsUniformBuffer = device->CreateBuffer(&settingsBufferDesc);
        const auto data = glm::vec3(grassSettings.sideLength,
                                    grassSettings.density,
                                    static_cast<float_t>(grassSettings.sideLength) * 2.0 / static_cast<float_t>(
                                        grassSettings.bladesPerSide));
        queue->WriteBuffer(grassSettingsUniformBuffer, 0, &data, grassSettingsUniformBuffer.GetSize());
    }


    void ComputeManager::initComputPipeline()
    {
        wgpu::ShaderModule computeModule = getShaderModule(*device, "../shaders/grass_position.compute.wgsl",
                                                           "Grass position compute module");
        wgpu::ComputePipelineDescriptor computePipelineDesc;
        computePipelineDesc.label = "Compute pipeline";

        wgpu::ProgrammableStageDescriptor computeStageDesc = {
            .module = computeModule,
            .entryPoint = "main"
        };
        computePipelineDesc.compute = computeStageDesc;


        wgpu::BindGroupLayoutEntry entryLayouts[2] = {
            {
                .binding = 0,
                .visibility = wgpu::ShaderStage::Compute,
                .buffer = {
                    .type = wgpu::BufferBindingType::Uniform,
                    .minBindingSize = sizeof(glm::vec3)
                }
            },
            {
                .binding = 1,
                .visibility = wgpu::ShaderStage::Compute,
                .buffer = {
                    .type = wgpu::BufferBindingType::Storage,
                    .minBindingSize = bladesPosBuffer.GetSize()
                }
            }
        };


        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc = {
            .entryCount = 2,
            .entries = &entryLayouts[0]
        };
        wgpu::BindGroupLayout bindGroupLayout = device->CreateBindGroupLayout(&bindGroupLayoutDesc);

        wgpu::PipelineLayoutDescriptor pipelineLayoutDesc = {
            .bindGroupLayoutCount = 1,
            .bindGroupLayouts = &bindGroupLayout
        };
        wgpu::PipelineLayout pipelineLayout = device->CreatePipelineLayout(&pipelineLayoutDesc);
        computePipelineDesc.layout = pipelineLayout;

        computePipeline = device->CreateComputePipeline(&computePipelineDesc);


        wgpu::BindGroupEntry entries[2] = {
            {
                .binding = 0,
                .buffer = grassSettingsUniformBuffer,
                .offset = 0,
                .size = grassSettingsUniformBuffer.GetSize(),
            },
            {
                .binding = 1,
                .buffer = bladesPosBuffer,
                .offset = 0,
                .size = bladesPosBuffer.GetSize(),
            }
        };

        wgpu::BindGroupDescriptor bindGroupDesc = {
            .label = "Compute bind group",
            .layout = bindGroupLayout,
            .entryCount = 2,
            .entries = &entries[0]
        };
        computeBindGroup = device->CreateBindGroup(&bindGroupDesc);
    }


    void ComputeManager::compute(const GrassSettings& grassSettings)
    {
        wgpu::CommandEncoderDescriptor encoderDesc;
        wgpu::CommandEncoder encoder = device->CreateCommandEncoder(&encoderDesc);

        wgpu::ComputePassDescriptor computePassDesc;
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&computePassDesc);

        pass.SetPipeline(computePipeline);
        pass.SetBindGroup(0, computeBindGroup);
        pass.DispatchWorkgroups(grassSettings.bladesPerSide, grassSettings.bladesPerSide, 1);
        pass.End();

        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {
            .label = "Compute operations command buffer"
        };
        // Submit the command buffer
        wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);
        queue->Submit(1, &command);

        device->Tick();
    }
} // grass
