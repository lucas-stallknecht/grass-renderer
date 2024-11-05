#include "ComputeManager.h"

#include "Utils.h"
#include <iostream>

namespace grass
{
    ComputeManager::ComputeManager(std::shared_ptr<Context> ctx) : ctx(std::move(ctx))
    {
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
            .size = sizeof(Blade) * ctx->totalBlades,
            .mappedAtCreation = false
        };
        computeBuffer = ctx->device.CreateBuffer(&sharedComputeBufferDesc);
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
        wgpu::BindGroupLayout sharedLayout = ctx->device.CreateBindGroupLayout(&sharedBindGroupLayoutDesc);


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
        sharedBindGroup = ctx->device.CreateBindGroup(&sharedBindGroupDesc);

        return sharedLayout;
    }


    void ComputeManager::createUniformBuffers()
    {
        wgpu::BufferDescriptor genSettingsBufferDesc = {
            .label = "Gen settings uniform buffer",
            .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
            .size = sizeof(ctx->grassUniform),
            .mappedAtCreation = false
        };
        genSettingsUniformBuffer = ctx->device.CreateBuffer(&genSettingsBufferDesc);

        wgpu::BufferDescriptor movSettingsBufferDesc = {
            .label = "Mov settings uniform buffer",
            .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
            .size = sizeof(ctx->movUniform),
            .mappedAtCreation = false
        };
        movSettingsUniformBuffer = ctx->device.CreateBuffer(&movSettingsBufferDesc);
        ctx->queue.WriteBuffer(movSettingsUniformBuffer, 0, &ctx->movUniform, movSettingsUniformBuffer.GetSize());

        wgpu::BufferDescriptor movDynamicBufferDesc = {
            .label = "Mov dynamic uniform buffer",
            .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
            .size = sizeof(float),
            .mappedAtCreation = false
        };
        movDynamicUniformBuffer = ctx->device.CreateBuffer(&movDynamicBufferDesc);
    }


    void ComputeManager::initGenPipeline(const wgpu::BindGroupLayout& sharedLayout)
    {
        const wgpu::ShaderModule genModule = getShaderModule(ctx->device, "../shaders/gen.compute.wgsl",
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
                .minBindingSize = sizeof(ctx->grassUniform)
            }
        };
        wgpu::BindGroupLayoutDescriptor genBindGroupLayoutDesc = {
            .entryCount = 1,
            .entries = &genEntryLayout
        };
        wgpu::BindGroupLayout genBindGroupLayout = ctx->device.CreateBindGroupLayout(&genBindGroupLayoutDesc);

        wgpu::BindGroupLayout bindGroupLayouts[2] = {
            sharedLayout, genBindGroupLayout
        };

        wgpu::PipelineLayoutDescriptor genPipelineLayoutDesc = {
            .bindGroupLayoutCount = 2,
            .bindGroupLayouts = &bindGroupLayouts[0]
        };
        genPipelineDesc.layout = ctx->device.CreatePipelineLayout(&genPipelineLayoutDesc);

        genPipeline = ctx->device.CreateComputePipeline(&genPipelineDesc);


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
        genBindGroup = ctx->device.CreateBindGroup(&bindGroupDesc);
    }


    void ComputeManager::initMovPipeline(const wgpu::BindGroupLayout& sharedLayout)
    {
        const wgpu::ShaderModule movModule = getShaderModule(ctx->device, "../shaders/move.compute.wgsl",
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
                    .minBindingSize = sizeof(ctx->movUniform)
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
        wgpu::BindGroupLayout movBindGroupLayout = ctx->device.CreateBindGroupLayout(&movBindGroupLayoutDesc);

        wgpu::BindGroupLayout bindGroupLayouts[2] = {
            sharedLayout, movBindGroupLayout
        };

        wgpu::PipelineLayoutDescriptor movPipelineLayoutDesc = {
            .bindGroupLayoutCount = 2,
            .bindGroupLayouts = &bindGroupLayouts[0]
        };
        movPipelineDesc.layout = ctx->device.CreatePipelineLayout(&movPipelineLayoutDesc);

        movPipeline = ctx->device.CreateComputePipeline(&movPipelineDesc);

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
        movBindGroup = ctx->device.CreateBindGroup(&bindGroupDesc);
    }


    void ComputeManager::updateMovSettingsUniorm()
    {
        ctx->queue.WriteBuffer(movSettingsUniformBuffer, 0, &ctx->movUniform, movSettingsUniformBuffer.GetSize());
    }


    void ComputeManager::generate()
    {
        std::cout << ctx->grassUniform.bladeHeight << std::endl;
        ctx->queue.WriteBuffer(genSettingsUniformBuffer, 0, &ctx->grassUniform, genSettingsUniformBuffer.GetSize());
        wgpu::CommandEncoderDescriptor encoderDesc;
        wgpu::CommandEncoder encoder = ctx->device.CreateCommandEncoder(&encoderDesc);

        wgpu::ComputePassDescriptor computePassDesc;
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&computePassDesc);

        pass.SetPipeline(genPipeline);
        pass.SetBindGroup(0, sharedBindGroup);
        pass.SetBindGroup(1, genBindGroup);
        pass.DispatchWorkgroups(ctx->bladesPerSide, ctx->bladesPerSide, 1);
        pass.End();

        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {
            .label = "Compute operations command buffer"
        };
        // Submit the command buffer
        wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);
        ctx->queue.Submit(1, &command);
    }

    void ComputeManager::computeMovement(float time)
    {
        ctx->queue.WriteBuffer(movDynamicUniformBuffer, 0, &time, movDynamicUniformBuffer.GetSize());
        wgpu::CommandEncoderDescriptor encoderDesc;
        wgpu::CommandEncoder encoder = ctx->device.CreateCommandEncoder(&encoderDesc);

        wgpu::ComputePassDescriptor computePassDesc;
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&computePassDesc);

        pass.SetPipeline(movPipeline);
        pass.SetBindGroup(0, sharedBindGroup);
        pass.SetBindGroup(1, movBindGroup);
        pass.DispatchWorkgroups(ctx->bladesPerSide, ctx->bladesPerSide, 1);
        pass.End();

        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {
            .label = "Compute operations command buffer"
        };
        // Submit the command buffer
        wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);
        ctx->queue.Submit(1, &command);
    }
} // grass
