#include "ComputeManager.h"

#include "Utils.h"
#include <iostream>

namespace grass
{
    ComputeManager::ComputeManager(std::shared_ptr<wgpu::Device> device, std::shared_ptr<wgpu::Queue> queue)
        : device(std::move(device)), queue(std::move(queue))
    {
    }

    wgpu::Buffer ComputeManager::init(const GrassGenerationSettings& genSettings,
                                      const GrassMovUniformData& movSettings)
    {
        createSharedBuffer(genSettings);
        wgpu::BindGroupLayout sharedLayout = createSharedBindGroup();
        createUniformBuffers(genSettings, movSettings);
        initGenPipeline(genSettings, sharedLayout);
        initMovPipeline(movSettings, sharedLayout);

        return computeBuffer;
    }


    void ComputeManager::createSharedBuffer(const GrassGenerationSettings& genSettings)
    {
        wgpu::BufferDescriptor sharedComputeBufferDesc = {
            .label = "Grass blade instance info storage buffer",
            .usage = wgpu::BufferUsage::Storage,
            .size = sizeof(Blade) * genSettings.totalBlades,
            .mappedAtCreation = false
        };
        computeBuffer = device->CreateBuffer(&sharedComputeBufferDesc);
    }


    wgpu::BindGroupLayout ComputeManager::createSharedBindGroup()
    {
        wgpu::BindGroupLayoutEntry entryLayout = {
            .binding = 0,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = computeBuffer.GetSize()
            }
        };
        wgpu::BindGroupLayoutDescriptor sharedBindGroupLayoutDesc = {
            .entryCount = 1,
            .entries = &entryLayout
        };
        wgpu::BindGroupLayout sharedLayout = device->CreateBindGroupLayout(&sharedBindGroupLayoutDesc);


        wgpu::BindGroupEntry entry = {
            .binding = 0,
            .buffer = computeBuffer,
            .offset = 0,
            .size = computeBuffer.GetSize()
        };

        wgpu::BindGroupDescriptor sharedBindGroupDesc = {
            .layout = sharedLayout,
            .entryCount = 1,
            .entries = &entry
        };
        sharedBindGroup = device->CreateBindGroup(&sharedBindGroupDesc);

        return sharedLayout;
    }


    void ComputeManager::createUniformBuffers(const GrassGenerationSettings& genSettings,
                                              const GrassMovUniformData& movUniformData)
    {
        wgpu::BufferDescriptor genSettingsBufferDesc = {
            .label = "Gen settings uniform buffer",
            .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
            .size = sizeof(genSettings.grassUniform),
            .mappedAtCreation = false
        };
        genSettingsUniformBuffer = device->CreateBuffer(&genSettingsBufferDesc);

        wgpu::BufferDescriptor movSettingsBufferDesc = {
            .label = "Mov settings uniform buffer",
            .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
            .size = sizeof(movUniformData),
            .mappedAtCreation = false
        };
        movSettingsUniformBuffer = device->CreateBuffer(&movSettingsBufferDesc);
        queue->WriteBuffer(movSettingsUniformBuffer, 0, &movUniformData, movSettingsUniformBuffer.GetSize());

        wgpu::BufferDescriptor movDynamicBufferDesc = {
            .label = "Mov dynamic uniform buffer",
            .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
            .size = sizeof(float),
            .mappedAtCreation = false
        };
        movDynamicUniformBuffer = device->CreateBuffer(&movDynamicBufferDesc);
    }


    void ComputeManager::initGenPipeline(const GrassGenerationSettings& genSettings,
                                         const wgpu::BindGroupLayout& sharedLayout)
    {
        const wgpu::ShaderModule genModule = getShaderModule(*device, "../shaders/gen.compute.wgsl",
                                                             "Grass generation compute module");
        wgpu::ComputePipelineDescriptor genPipelineDesc;
        genPipelineDesc.label = "Generation compute pipeline";

        wgpu::ProgrammableStageDescriptor genStageDesc = {
            .module = genModule,
            .entryPoint = "main"
        };
        genPipelineDesc.compute = genStageDesc;


        wgpu::BindGroupLayoutEntry genEntryLayout = {
            .binding = 0,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Uniform,
                .minBindingSize = sizeof(genSettings.grassUniform)
            }
        };
        wgpu::BindGroupLayoutDescriptor genBindGroupLayoutDesc = {
            .entryCount = 1,
            .entries = &genEntryLayout
        };
        wgpu::BindGroupLayout genBindGroupLayout = device->CreateBindGroupLayout(&genBindGroupLayoutDesc);

        wgpu::BindGroupLayout bindGroupLayouts[2] = {
            sharedLayout, genBindGroupLayout
        };

        wgpu::PipelineLayoutDescriptor genPipelineLayoutDesc = {
            .bindGroupLayoutCount = 2,
            .bindGroupLayouts = &bindGroupLayouts[0]
        };
        genPipelineDesc.layout = device->CreatePipelineLayout(&genPipelineLayoutDesc);

        genPipeline = device->CreateComputePipeline(&genPipelineDesc);


        wgpu::BindGroupEntry genEntry =
        {
            .binding = 0,
            .buffer = genSettingsUniformBuffer,
            .offset = 0,
            .size = genSettingsUniformBuffer.GetSize(),
        };

        wgpu::BindGroupDescriptor bindGroupDesc = {
            .label = "Generation uniform bind group",
            .layout = genBindGroupLayout,
            .entryCount = 1,
            .entries = &genEntry
        };
        genBindGroup = device->CreateBindGroup(&bindGroupDesc);
    }


    void ComputeManager::initMovPipeline(const GrassMovUniformData& movSettings,
                                         const wgpu::BindGroupLayout& sharedLayout)
    {
        const wgpu::ShaderModule movModule = getShaderModule(*device, "../shaders/move.compute.wgsl",
                                                             "Grass movement compute module");
        wgpu::ComputePipelineDescriptor movPipelineDesc;
        movPipelineDesc.label = "Movement compute pipeline";

        wgpu::ProgrammableStageDescriptor movStageDesc = {
            .module = movModule,
            .entryPoint = "main"
        };
        movPipelineDesc.compute = movStageDesc;


        wgpu::BindGroupLayoutEntry movEntryLayouts[2] = {
            {
                .binding = 0,
                .visibility = wgpu::ShaderStage::Compute,
                .buffer = {
                    .type = wgpu::BufferBindingType::Uniform,
                    .minBindingSize = sizeof(movSettings)
                }
            },
            {
                .binding = 1,
                .visibility = wgpu::ShaderStage::Compute,
                .buffer = {
                    .type = wgpu::BufferBindingType::Uniform,
                    .minBindingSize = sizeof(float)
                }
            }
        };
        wgpu::BindGroupLayoutDescriptor movBindGroupLayoutDesc = {
            .entryCount = 2,
            .entries = &movEntryLayouts[0]
        };
        wgpu::BindGroupLayout movBindGroupLayout = device->CreateBindGroupLayout(&movBindGroupLayoutDesc);

        wgpu::BindGroupLayout bindGroupLayouts[2] = {
            sharedLayout, movBindGroupLayout
        };

        wgpu::PipelineLayoutDescriptor movPipelineLayoutDesc = {
            .bindGroupLayoutCount = 2,
            .bindGroupLayouts = &bindGroupLayouts[0]
        };
        movPipelineDesc.layout = device->CreatePipelineLayout(&movPipelineLayoutDesc);

        movPipeline = device->CreateComputePipeline(&movPipelineDesc);

        wgpu::BindGroupEntry movEntries[2] = {
            {
                .binding = 0,
                .buffer = movSettingsUniformBuffer,
                .offset = 0,
                .size = movSettingsUniformBuffer.GetSize(),
            },
            {
                .binding = 1,
                .buffer = movDynamicUniformBuffer,
                .offset = 0,
                .size = movDynamicUniformBuffer.GetSize(),
            },
        };

        wgpu::BindGroupDescriptor bindGroupDesc = {
            .label = "Movement uniform bind group",
            .layout = movBindGroupLayout,
            .entryCount = 2,
            .entries = &movEntries[0]
        };
        movBindGroup = device->CreateBindGroup(&bindGroupDesc);
    }


    void ComputeManager::updateMovSettingsUniorm(const GrassMovUniformData& movSettings)
    {
        queue->WriteBuffer(movSettingsUniformBuffer, 0, &movSettings, movSettingsUniformBuffer.GetSize());
    }


    void ComputeManager::generate(const GrassGenerationSettings& genSettings)
    {
        queue->WriteBuffer(genSettingsUniformBuffer, 0, &genSettings.grassUniform, genSettingsUniformBuffer.GetSize());
        wgpu::CommandEncoderDescriptor encoderDesc;
        wgpu::CommandEncoder encoder = device->CreateCommandEncoder(&encoderDesc);

        wgpu::ComputePassDescriptor computePassDesc;
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&computePassDesc);

        pass.SetPipeline(genPipeline);
        pass.SetBindGroup(0, sharedBindGroup);
        pass.SetBindGroup(1, genBindGroup);
        pass.DispatchWorkgroups(genSettings.bladesPerSide, genSettings.bladesPerSide, 1);
        pass.End();

        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {
            .label = "Compute operations command buffer"
        };
        // Submit the command buffer
        wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);
        queue->Submit(1, &command);
    }

    void ComputeManager::computeMovement(const GrassGenerationSettings& genSettings, float time)
    {
        queue->WriteBuffer(movDynamicUniformBuffer, 0, &time, movDynamicUniformBuffer.GetSize());
        wgpu::CommandEncoderDescriptor encoderDesc;
        wgpu::CommandEncoder encoder = device->CreateCommandEncoder(&encoderDesc);

        wgpu::ComputePassDescriptor computePassDesc;
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&computePassDesc);

        pass.SetPipeline(movPipeline);
        pass.SetBindGroup(0, sharedBindGroup);
        pass.SetBindGroup(1, movBindGroup);
        pass.DispatchWorkgroups(genSettings.bladesPerSide, genSettings.bladesPerSide, 1);
        pass.End();

        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {
            .label = "Compute operations command buffer"
        };
        // Submit the command buffer
        wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);
        queue->Submit(1, &command);
    }
} // grass
