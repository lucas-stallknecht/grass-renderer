#include "ComputeManager.h"

#include "Utils.h"
#include <iostream>

namespace grass
{
    ComputeManager::ComputeManager(std::shared_ptr<GlobalConfig> config) : config(std::move(config))
    {
        ctx = GPUContext::getInstance();
    }

    wgpu::Buffer ComputeManager::init()
    {
        createSharedBuffer();
        wgpu::BindGroupLayout sharedLayout = createSharedBindGroup();
        createUniformBuffers();
        initGenPipeline(sharedLayout);
        initMovPipeline(sharedLayout);

        return computeBuffer;
    }


    void ComputeManager::createSharedBuffer()
    {
        wgpu::BufferDescriptor sharedComputeBufferDesc = {
            .label = "Grass blade instance info storage buffer",
            .usage = wgpu::BufferUsage::Storage,
            .size = sizeof(Blade) * config->totalBlades,
            .mappedAtCreation = false
        };
        computeBuffer = ctx->getDevice().CreateBuffer(&sharedComputeBufferDesc);
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
        wgpu::BindGroupLayout sharedLayout = ctx->getDevice().CreateBindGroupLayout(&sharedBindGroupLayoutDesc);


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
        sharedBindGroup = ctx->getDevice().CreateBindGroup(&sharedBindGroupDesc);

        return sharedLayout;
    }


    void ComputeManager::createUniformBuffers()
    {
        wgpu::BufferDescriptor genSettingsBufferDesc = {
            .label = "Gen settings uniform buffer",
            .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
            .size = sizeof(config->grassUniform),
            .mappedAtCreation = false
        };
        genSettingsUniformBuffer = ctx->getDevice().CreateBuffer(&genSettingsBufferDesc);

        wgpu::BufferDescriptor movSettingsBufferDesc = {
            .label = "Mov settings uniform buffer",
            .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
            .size = sizeof(config->movUniform),
            .mappedAtCreation = false
        };
        movSettingsUniformBuffer = ctx->getDevice().CreateBuffer(&movSettingsBufferDesc);
        ctx->getQueue().WriteBuffer(movSettingsUniformBuffer, 0, &config->movUniform, movSettingsUniformBuffer.GetSize());

        wgpu::BufferDescriptor movDynamicBufferDesc = {
            .label = "Mov dynamic uniform buffer",
            .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
            .size = sizeof(float),
            .mappedAtCreation = false
        };
        movDynamicUniformBuffer = ctx->getDevice().CreateBuffer(&movDynamicBufferDesc);
    }


    void ComputeManager::initGenPipeline(const wgpu::BindGroupLayout& sharedLayout)
    {
        const wgpu::ShaderModule genModule = getShaderModule(ctx->getDevice(), "../shaders/gen.compute.wgsl",
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
                .minBindingSize = sizeof(config->grassUniform)
            }
        };
        wgpu::BindGroupLayoutDescriptor genBindGroupLayoutDesc = {
            .entryCount = 1,
            .entries = &genEntryLayout
        };
        wgpu::BindGroupLayout genBindGroupLayout = ctx->getDevice().CreateBindGroupLayout(&genBindGroupLayoutDesc);

        wgpu::BindGroupLayout bindGroupLayouts[2] = {
            sharedLayout, genBindGroupLayout
        };

        wgpu::PipelineLayoutDescriptor genPipelineLayoutDesc = {
            .bindGroupLayoutCount = 2,
            .bindGroupLayouts = &bindGroupLayouts[0]
        };
        genPipelineDesc.layout = ctx->getDevice().CreatePipelineLayout(&genPipelineLayoutDesc);

        genPipeline = ctx->getDevice().CreateComputePipeline(&genPipelineDesc);


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
        genBindGroup = ctx->getDevice().CreateBindGroup(&bindGroupDesc);
    }


    void ComputeManager::initMovPipeline(const wgpu::BindGroupLayout& sharedLayout)
    {
        const wgpu::ShaderModule movModule = getShaderModule(ctx->getDevice(), "../shaders/move.compute.wgsl",
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
                    .minBindingSize = sizeof(config->movUniform)
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
        wgpu::BindGroupLayout movBindGroupLayout = ctx->getDevice().CreateBindGroupLayout(&movBindGroupLayoutDesc);

        wgpu::BindGroupLayout bindGroupLayouts[2] = {
            sharedLayout, movBindGroupLayout
        };

        wgpu::PipelineLayoutDescriptor movPipelineLayoutDesc = {
            .bindGroupLayoutCount = 2,
            .bindGroupLayouts = &bindGroupLayouts[0]
        };
        movPipelineDesc.layout = ctx->getDevice().CreatePipelineLayout(&movPipelineLayoutDesc);

        movPipeline = ctx->getDevice().CreateComputePipeline(&movPipelineDesc);

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
        movBindGroup = ctx->getDevice().CreateBindGroup(&bindGroupDesc);
    }


    void ComputeManager::updateMovSettingsUniorm()
    {
        ctx->getQueue().WriteBuffer(movSettingsUniformBuffer, 0, &config->movUniform, movSettingsUniformBuffer.GetSize());
    }


    void ComputeManager::generate()
    {
        ctx->getQueue().WriteBuffer(genSettingsUniformBuffer, 0, &config->grassUniform, genSettingsUniformBuffer.GetSize());
        wgpu::CommandEncoderDescriptor encoderDesc;
        wgpu::CommandEncoder encoder = ctx->getDevice().CreateCommandEncoder(&encoderDesc);

        wgpu::ComputePassDescriptor computePassDesc;
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&computePassDesc);

        pass.SetPipeline(genPipeline);
        pass.SetBindGroup(0, sharedBindGroup);
        pass.SetBindGroup(1, genBindGroup);
        pass.DispatchWorkgroups(config->bladesPerSide, config->bladesPerSide, 1);
        pass.End();

        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {
            .label = "Compute operations command buffer"
        };
        // Submit the command buffer
        wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);
        ctx->getQueue().Submit(1, &command);
    }

    void ComputeManager::computeMovement(float time)
    {
        ctx->getQueue().WriteBuffer(movDynamicUniformBuffer, 0, &time, movDynamicUniformBuffer.GetSize());
        wgpu::CommandEncoderDescriptor encoderDesc;
        wgpu::CommandEncoder encoder = ctx->getDevice().CreateCommandEncoder(&encoderDesc);

        wgpu::ComputePassDescriptor computePassDesc;
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&computePassDesc);

        pass.SetPipeline(movPipeline);
        pass.SetBindGroup(0, sharedBindGroup);
        pass.SetBindGroup(1, movBindGroup);
        pass.DispatchWorkgroups(config->bladesPerSide, config->bladesPerSide, 1);
        pass.End();

        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {
            .label = "Compute operations command buffer"
        };
        // Submit the command buffer
        wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);
        ctx->getQueue().Submit(1, &command);
    }
} // grass
