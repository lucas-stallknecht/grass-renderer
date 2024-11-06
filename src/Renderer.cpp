#include "Renderer.h"
#include "Utils.h"

#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>
#include "layouts.h"

#include <iostream>

namespace grass
{
    Renderer::Renderer(std::shared_ptr<GlobalConfig> config, const uint16_t width, const uint16_t height)
        : config(std::move(config)), size(wgpu::Extent2D{width, height})
    {
        ctx = GPUContext::getInstance();
    }


    void Renderer::init(const wgpu::Buffer& computeBuffer)
    {
        initGlobalResources();
        initBladeResources();
        initGrassPipeline(computeBuffer);
        initPhongPipeline();
        createDepthTextureView();
    }


    void Renderer::initGlobalResources()
    {
        wgpu::BufferDescriptor globalUniformBufferDesc = {
            .label = "Global uniform buffer",
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
            .size = sizeof(GlobalUniformData),
            .mappedAtCreation = false,
        };
        globalUniformBuffer = ctx->getDevice().CreateBuffer(&globalUniformBufferDesc);

        wgpu::SamplerDescriptor samplerDesc = {};
        textureSampler = ctx->getDevice().CreateSampler(&samplerDesc);
    }


    void Renderer::initBladeResources()
    {
        bladeNormalTexture = loadTexture("../assets/blade_normal.png");

        wgpu::BufferDescriptor bladeUniformBufferDesc = {
            .label = "Blade : static uniform buffer",
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
            .size = sizeof(BladeStaticUniformData),
            .mappedAtCreation = false,
        };
        bladeUniformBuffer = ctx->getDevice().CreateBuffer(&bladeUniformBufferDesc);
    }


    void Renderer::initGrassPipeline(const wgpu::Buffer& computeBuffer)
    {
        wgpu::ShaderModule vertModule = getShaderModule(ctx->getDevice(), "../shaders/blade.vert.wgsl",
                                                        "Grass Vertex Module");
        wgpu::ShaderModule fragModule = getShaderModule(ctx->getDevice(), "../shaders/blade.frag.wgsl",
                                                        "Grass Frag Module");

        wgpu::RenderPipelineDescriptor pipelineDesc;

        wgpu::BindGroupLayoutEntry grassEntryLayouts[4] = {
            {
                .binding = 0,
                .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
                .buffer = {
                    .type = wgpu::BufferBindingType::Uniform,
                    .minBindingSize = bladeUniformBuffer.GetSize()
                }
            },
            {
                .binding = 1,
                .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
                .texture = {
                    .sampleType = wgpu::TextureSampleType::Float,
                    .viewDimension = wgpu::TextureViewDimension::e2D
                },
            },
            {
                .binding = 2,
                .visibility = wgpu::ShaderStage::Vertex,
                .buffer = {
                    .type = wgpu::BufferBindingType::ReadOnlyStorage,
                    .minBindingSize = computeBuffer.GetSize()
                }
            }
        };

        wgpu::BindGroupLayoutDescriptor grassBindGroupLayoutDesc = {
            .label = "Storage bind group layout",
            .entryCount = 3,
            .entries = &grassEntryLayouts[0]
        };

        wgpu::BindGroupLayout globalBindGroupLayout = ctx->getDevice().CreateBindGroupLayout(
            &globalBindGroupLayoutDesc);
        wgpu::BindGroupLayout grassBindGroupLayout = ctx->getDevice().CreateBindGroupLayout(&grassBindGroupLayoutDesc);


        wgpu::BindGroupLayout bindGroupLayouts[2] = {
            globalBindGroupLayout, grassBindGroupLayout
        };
        wgpu::PipelineLayoutDescriptor lipelineLayoutDesc = {
            .bindGroupLayoutCount = 2,
            .bindGroupLayouts = &bindGroupLayouts[0]
        };
        wgpu::PipelineLayout pipelineLayout = ctx->getDevice().CreatePipelineLayout(&lipelineLayoutDesc);
        pipelineDesc.layout = pipelineLayout;


        // --- Attribute binding ---
        pipelineDesc.vertex.module = vertModule;
        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = &defaultVertexLayout;


        // --- Fragment state ---
        wgpu::ColorTargetState colorTarget;
        colorTarget.format = ctx->getSurfaceFormat();

        wgpu::FragmentState fragmentState = {
            .module = fragModule,
            .targetCount = 1,
            .targets = &colorTarget
        };
        pipelineDesc.fragment = &fragmentState;

        pipelineDesc.depthStencil = &defaultDepthStencil;

        pipelineDesc.label = "Blade of grass pipeline";
        grassPipeline = ctx->getDevice().CreateRenderPipeline(&pipelineDesc);

        // TEMPORARY
        wgpu::TextureViewDescriptor textureViewDesc = {
            .format = bladeNormalTexture.GetFormat(),
            .dimension = wgpu::TextureViewDimension::e2D,
            .mipLevelCount = 1,
            .arrayLayerCount = 1
        };
        wgpu::TextureView textureView = bladeNormalTexture.CreateView(&textureViewDesc);


        // --- Bind group ---
        // The "real" bindings between the buffer and the shader locs, not just the layout
        wgpu::BindGroupEntry globalEntries[2] = {
            {
                .binding = 0,
                .buffer = globalUniformBuffer,
                .offset = 0,
                .size = globalUniformBuffer.GetSize()
            },
            {
                .binding = 1,
                .sampler = textureSampler,
            },
        };
        wgpu::BindGroupDescriptor globalBindGroupDesc = {
            .label = "Global uniforms bind group",
            .layout = globalBindGroupLayout,
            .entryCount = globalBindGroupLayoutDesc.entryCount,
            .entries = &globalEntries[0]
        };
        globalBindGroup = ctx->getDevice().CreateBindGroup(&globalBindGroupDesc);

        wgpu::BindGroupEntry storageEntries[4] = {
            {
                .binding = 0,
                .buffer = bladeUniformBuffer,
                .offset = 0,
                .size = bladeUniformBuffer.GetSize()
            },
            {
                .binding = 1,
                .textureView = textureView,
            },
            {
                .binding = 2,
                .buffer = computeBuffer,
                .offset = 0,
                .size = computeBuffer.GetSize()
            },
        };
        wgpu::BindGroupDescriptor storageBindGroupDesc = {
            .label = "Storage bind group",
            .layout = grassBindGroupLayout,
            .entryCount = grassBindGroupLayoutDesc.entryCount,
            .entries = &storageEntries[0]
        };
        grassBindGroup = ctx->getDevice().CreateBindGroup(&storageBindGroupDesc);
    }


    void Renderer::initPhongPipeline()
    {
        wgpu::ShaderModule vertModule = getShaderModule(ctx->getDevice(), "../shaders/phong.vert.wgsl",
                                                        "Phong Vertex Module");
        wgpu::ShaderModule fragModule = getShaderModule(ctx->getDevice(), "../shaders/phong.frag.wgsl",
                                                        "Phong Frag Module");

        wgpu::RenderPipelineDescriptor pipelineDesc;


        wgpu::BindGroupLayout bindGroupLayouts[3] = {
            ctx->getDevice().CreateBindGroupLayout(&globalBindGroupLayoutDesc),
            ctx->getDevice().CreateBindGroupLayout(&phongMaterialBindGroupLayoutDesc),
            ctx->getDevice().CreateBindGroupLayout(&modelBindGroupLayoutDesc)
        };
        wgpu::PipelineLayoutDescriptor lipelineLayoutDesc = {
            .bindGroupLayoutCount = 3,
            .bindGroupLayouts = &bindGroupLayouts[0]
        };
        wgpu::PipelineLayout pipelineLayout = ctx->getDevice().CreatePipelineLayout(&lipelineLayoutDesc);
        pipelineDesc.layout = pipelineLayout;

        pipelineDesc.vertex.module = vertModule;
        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = &defaultVertexLayout;

        wgpu::ColorTargetState colorTarget;
        colorTarget.format = ctx->getSurfaceFormat();

        wgpu::FragmentState fragmentState = {
            .module = fragModule,
            .targetCount = 1,
            .targets = &colorTarget
        };
        pipelineDesc.fragment = &fragmentState;

        pipelineDesc.depthStencil = &defaultDepthStencil;

        pipelineDesc.label = "Phong pipeline";
        phongPipeline = ctx->getDevice().CreateRenderPipeline(&pipelineDesc);

        // TEMPORARY
        wgpu::TextureViewDescriptor textureViewDesc = {
            .format = bladeNormalTexture.GetFormat(),
            .dimension = wgpu::TextureViewDimension::e2D,
            .mipLevelCount = 1,
            .arrayLayerCount = 1
        };
        wgpu::TextureView textureView = bladeNormalTexture.CreateView(&textureViewDesc);
    }


    void Renderer::createDepthTextureView()
    {
        wgpu::TextureDescriptor depthTextureDesc = {
            .usage = wgpu::TextureUsage::RenderAttachment,
            .dimension = wgpu::TextureDimension::e2D,
            .size = {size.width, size.height, 1},
            .format = wgpu::TextureFormat::Depth24Plus,
            .mipLevelCount = 1,
            .sampleCount = 1,
        };
        wgpu::Texture depthTexture = ctx->getDevice().CreateTexture(&depthTextureDesc);

        wgpu::TextureViewDescriptor depthViewDesc = {
            .format = wgpu::TextureFormat::Depth24Plus,
            .dimension = wgpu::TextureViewDimension::e2D,
            .baseMipLevel = 0,
            .mipLevelCount = 1,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1,
            .aspect = wgpu::TextureAspect::DepthOnly,
        };
        depthView = depthTexture.CreateView(&depthViewDesc);
    }


    void Renderer::updateGlobalUniforms(const Camera& camera, float time)
    {
        CameraUniformData camUniforms = {
            camera.viewMatrix,
            camera.projMatrix,
            camera.position,
        };
        camUniforms.dir = camera.direction;
        GlobalUniformData globalUniforms = {
            camUniforms,
            time
        };
        ctx->getQueue().WriteBuffer(globalUniformBuffer, 0, &globalUniforms, globalUniformBuffer.GetSize());
    }


    void Renderer::updateBladeUniforms(const BladeStaticUniformData& uniform)
    {
        ctx->getQueue().WriteBuffer(bladeUniformBuffer, 0, &uniform, bladeUniformBuffer.GetSize());
    }


    void Renderer::drawScene(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView,
                             const std::vector<Mesh>& scene)
    {
        wgpu::RenderPassColorAttachment renderPassColorAttachment = {
            .view = targetView,
            .resolveTarget = nullptr,
            .loadOp = wgpu::LoadOp::Clear,
            .storeOp = wgpu::StoreOp::Store,
            .clearValue = wgpu::Color{0.1, 0.1, 0.1, 1.0},
        };
        wgpu::RenderPassDepthStencilAttachment renderPassDepthAttachment = {
            .view = depthView,
            .depthLoadOp = wgpu::LoadOp::Clear,
            .depthStoreOp = wgpu::StoreOp::Store,
            .depthClearValue = 1.0,
        };
        wgpu::RenderPassDescriptor renderPassDesc = {
            .colorAttachmentCount = 1,
            .colorAttachments = &renderPassColorAttachment,
            .depthStencilAttachment = &renderPassDepthAttachment,
        };

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
        pass.SetBindGroup(0, globalBindGroup, 0, nullptr);

        pass.SetPipeline(phongPipeline);
        for(auto mesh: scene)
        {
            pass.SetBindGroup(1, mesh.material.bindGroup, 0, nullptr);
            pass.SetBindGroup(2, mesh.bindGroup, 0, nullptr);
            mesh.draw(pass, 1);
        }

        pass.SetPipeline(grassPipeline);
        pass.SetBindGroup(1, grassBindGroup, 0, nullptr);
        bladeGeometry.draw(pass, config->totalBlades);

        pass.End();
    }

    void Renderer::drawGUI(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView)
    {
        wgpu::RenderPassColorAttachment imGuiColorAttachment = {
            .view = targetView,
            .loadOp = wgpu::LoadOp::Load,
            .storeOp = wgpu::StoreOp::Store,
        };
        wgpu::RenderPassDescriptor imGuiRenderPassDesc = {
            .label = "ImGui Render Pass",
            .colorAttachmentCount = 1,
            .colorAttachments = &imGuiColorAttachment,
        };
        wgpu::RenderPassEncoder imGuiPass = encoder.BeginRenderPass(&imGuiRenderPassDesc);
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), imGuiPass.Get());
        imGuiPass.End();
    }


    void Renderer::draw(const std::vector<Mesh>& scene)
    {
        wgpu::TextureView targetView = ctx->getNextSurfaceTextureView();
        if (!targetView) return;

        wgpu::CommandEncoderDescriptor encoderDesc;
        wgpu::CommandEncoder encoder = ctx->getDevice().CreateCommandEncoder(&encoderDesc);

        drawScene(encoder, targetView, scene);
        drawGUI(encoder, targetView);

        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {
            .label = "Rendering operations command buffer"
        };
        wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);
        ctx->getQueue().Submit(1, &command);
        ctx->getSurface().Present();
    }
} // grass
