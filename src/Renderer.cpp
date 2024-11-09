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
        initShadowResources();
        createDepthTextureView();
        initSkyPipeline();
        initGrassPipeline(computeBuffer);
        initPhongPipeline();
        initShadowPipeline();
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

        wgpu::SamplerDescriptor samplerDesc = {
            .magFilter = wgpu::FilterMode::Linear,
            .minFilter = wgpu::FilterMode::Linear,
        };
        globalTextureSampler = ctx->getDevice().CreateSampler(&samplerDesc);

        wgpu::BindGroupLayout globalBindGroupLayout = ctx->getDevice().
                                                           CreateBindGroupLayout(&globalBindGroupLayoutDesc);
        wgpu::BindGroupEntry globalEntry[globalBindGroupLayoutDesc.entryCount] = {
            {
                .binding = 0,
                .buffer = globalUniformBuffer,
                .offset = 0,
                .size = globalUniformBuffer.GetSize()
            },
            {
                .binding = 1,
                .sampler = globalTextureSampler,
            },
        };
        wgpu::BindGroupDescriptor globalBindGroupDesc = {
            .label = "Global uniforms bind group",
            .layout = globalBindGroupLayout,
            .entryCount = globalBindGroupLayoutDesc.entryCount,
            .entries = &globalEntry[0]
        };
        globalBindGroup = ctx->getDevice().CreateBindGroup(&globalBindGroupDesc);
    }


    void Renderer::initBladeResources()
    {
        bladeNormalTexture = loadTexture("../assets/blade_normal.png");

        wgpu::BufferDescriptor bladeUniformBufferDesc = {
            .label = "Blade uniform buffer",
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
            .size = sizeof(BladeStaticUniformData),
            .mappedAtCreation = false,
        };
        bladeUniformBuffer = ctx->getDevice().CreateBuffer(&bladeUniformBufferDesc);
        ctx->getQueue().WriteBuffer(bladeUniformBuffer, 0, &config->bladeUniform, bladeUniformBuffer.GetSize());
    }


    void Renderer::initShadowResources()
    {
        wgpu::BufferDescriptor shadowUniformBufferDesc = {
            .label = "Shadow uniform buffer",
            .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
            .size = sizeof(config->shadowUniform),
            .mappedAtCreation = false
        };
        shadowUniformBuffer = ctx->getDevice().CreateBuffer(&shadowUniformBufferDesc);
        ctx->getQueue().WriteBuffer(shadowUniformBuffer, 0, &config->shadowUniform, shadowUniformBuffer.GetSize());

        wgpu::TextureDescriptor shadowTextureDesc = {
            .usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::StorageBinding,
            .dimension = wgpu::TextureDimension::e2D,
            .size = {size.width, size.height, 1},
            .format = ctx->getSurfaceFormat(),
            .mipLevelCount = 1,
            .sampleCount = 1,
        };
        shadowTexture = ctx->getDevice().CreateTexture(&shadowTextureDesc);
    }


    void Renderer::initSkyPipeline()
    {
        wgpu::ShaderModule skyVert = getShaderModule(ctx->getDevice(), "../shaders/full_screen_quad.vert.wgsl",
                                                     "Sky Vertex shader");
        wgpu::ShaderModule skyFrag = getShaderModule(ctx->getDevice(), "../shaders/sky.frag.wgsl",
                                                     "Sky Frag shader");

        wgpu::BindGroupLayout bindGroupLayouts = ctx->getDevice().CreateBindGroupLayout(&globalBindGroupLayoutDesc);
        wgpu::PipelineLayoutDescriptor skyPipelineLayoutDesc = {
            .label = "Sky pipeline layout",
            .bindGroupLayoutCount = 1,
            .bindGroupLayouts = &bindGroupLayouts
        };
        wgpu::PipelineLayout pipelineLayout = ctx->getDevice().CreatePipelineLayout(&skyPipelineLayoutDesc);

        wgpu::RenderPipelineDescriptor skyPipelineDesc;
        skyPipelineDesc.label = "Sky pipeline";
        skyPipelineDesc.layout = pipelineLayout;
        skyPipelineDesc.vertex.module = skyVert;
        skyPipelineDesc.vertex.bufferCount = 1;
        skyPipelineDesc.vertex.buffers = &defaultVertexLayout;
        skyPipelineDesc.multisample.count = MULTI_SAMPLE_COUNT;

        wgpu::ColorTargetState colorTarget;
        colorTarget.format = ctx->getSurfaceFormat();

        wgpu::FragmentState fragmentState = {
            .module = skyFrag,
            .targetCount = 1,
            .targets = &colorTarget
        };
        skyPipelineDesc.fragment = &fragmentState;

        skyPipeline = ctx->getDevice().CreateRenderPipeline(&skyPipelineDesc);
    }


    void Renderer::initGrassPipeline(const wgpu::Buffer& computeBuffer)
    {
        wgpu::ShaderModule grassVert = getShaderModule(ctx->getDevice(), "../shaders/blade.vert.wgsl",
                                                       "Grass vertex shader");
        wgpu::ShaderModule fragVert = getShaderModule(ctx->getDevice(), "../shaders/blade.frag.wgsl",
                                                      "Grass vertex shader");


        wgpu::BindGroupLayoutEntry grassLayoutEntry[4] = {
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
                .visibility = wgpu::ShaderStage::Vertex,
                .buffer = {
                    .type = wgpu::BufferBindingType::ReadOnlyStorage,
                    .minBindingSize = computeBuffer.GetSize()
                }
            },
            {
                .binding = 2,
                .visibility = wgpu::ShaderStage::Fragment,
                .texture = {
                    .sampleType = wgpu::TextureSampleType::Float,
                    .viewDimension = wgpu::TextureViewDimension::e2D
                },
            },
            {
                .binding = 3,
                .visibility = wgpu::ShaderStage::Fragment,
                .texture = {
                    .sampleType = wgpu::TextureSampleType::Float,
                    .viewDimension = wgpu::TextureViewDimension::e2D
                }
            }
        };

        wgpu::BindGroupLayoutDescriptor bladeUniformBindGroupLayoutDesc = {
            .label = "Blade shading uniform bind group layout",
            .entryCount = 4,
            .entries = &grassLayoutEntry[0]
        };

        wgpu::BindGroupLayout globalBindGroupLayout = ctx->getDevice().
                                                           CreateBindGroupLayout(&globalBindGroupLayoutDesc);
        wgpu::BindGroupLayout bladeUniformBindGroupLayout = ctx->getDevice().CreateBindGroupLayout(
            &bladeUniformBindGroupLayoutDesc);

        wgpu::BindGroupLayout bindGroupLayouts[2] = {
            globalBindGroupLayout, bladeUniformBindGroupLayout
        };
        wgpu::PipelineLayoutDescriptor pipelineLayoutdesc = {
            .label = "Grass pipeline layout",
            .bindGroupLayoutCount = 2,
            .bindGroupLayouts = &bindGroupLayouts[0]
        };
        wgpu::PipelineLayout pipelineLayout = ctx->getDevice().CreatePipelineLayout(&pipelineLayoutdesc);

        wgpu::RenderPipelineDescriptor grassPipelineDesc;
        grassPipelineDesc.label = "Grass pipeline";
        grassPipelineDesc.layout = pipelineLayout;
        grassPipelineDesc.vertex.module = grassVert;
        grassPipelineDesc.vertex.bufferCount = 1;
        grassPipelineDesc.vertex.buffers = &defaultVertexLayout;

        wgpu::ColorTargetState colorTarget;
        colorTarget.format = ctx->getSurfaceFormat();

        wgpu::FragmentState fragmentState = {
            .module = fragVert,
            .targetCount = 1,
            .targets = &colorTarget
        };
        grassPipelineDesc.fragment = &fragmentState;

        grassPipelineDesc.multisample.count = MULTI_SAMPLE_COUNT;
        grassPipelineDesc.depthStencil = &defaultDepthStencil;

        grassPipeline = ctx->getDevice().CreateRenderPipeline(&grassPipelineDesc);

        wgpu::TextureViewDescriptor normalTextureViewDesc = {
            .format = bladeNormalTexture.GetFormat(),
            .dimension = wgpu::TextureViewDimension::e2D,
            .mipLevelCount = 1,
            .arrayLayerCount = 1
        };
        wgpu::TextureView normalTextureView = bladeNormalTexture.CreateView(&normalTextureViewDesc);

        wgpu::BindGroupEntry bladeUniformEntry[4] = {
            {
                .binding = 0,
                .buffer = bladeUniformBuffer,
                .offset = 0,
                .size = bladeUniformBuffer.GetSize()
            },
            {
                .binding = 1,
                .buffer = computeBuffer,
                .offset = 0,
                .size = computeBuffer.GetSize()
            },
            {
                .binding = 2,
                .textureView = normalTextureView,
            },
            {
                .binding = 3,
                .textureView = shadowTexture.CreateView(),
            },

        };
        wgpu::BindGroupDescriptor storageBindGroupDesc = {
            .label = "Blade uniform bind group",
            .layout = bladeUniformBindGroupLayout,
            .entryCount = bladeUniformBindGroupLayoutDesc.entryCount,
            .entries = &bladeUniformEntry[0]
        };
        bladeUniformBindGroup = ctx->getDevice().CreateBindGroup(&storageBindGroupDesc);
    }


    void Renderer::initPhongPipeline()
    {
        wgpu::ShaderModule phongVert = getShaderModule(ctx->getDevice(), "../shaders/phong.vert.wgsl",
                                                       "Phong vertex shader");
        wgpu::ShaderModule phongFrag = getShaderModule(ctx->getDevice(), "../shaders/phong.frag.wgsl",
                                                       "Phong frag shader");

        wgpu::RenderPipelineDescriptor phongPipelineDesc;

        wgpu::BindGroupLayout bindGroupLayouts[3] = {
            ctx->getDevice().CreateBindGroupLayout(&globalBindGroupLayoutDesc),
            ctx->getDevice().CreateBindGroupLayout(&phongMaterialBindGroupLayoutDesc),
            ctx->getDevice().CreateBindGroupLayout(&modelBindGroupLayoutDesc)
        };
        wgpu::PipelineLayoutDescriptor pipelineLayoutDesc = {
            .label = "Phong pipeline layout",
            .bindGroupLayoutCount = 3,
            .bindGroupLayouts = &bindGroupLayouts[0]
        };
        wgpu::PipelineLayout pipelineLayout = ctx->getDevice().CreatePipelineLayout(&pipelineLayoutDesc);
        phongPipelineDesc.label = "Phong pipeline";
        phongPipelineDesc.layout = pipelineLayout;
        phongPipelineDesc.vertex.module = phongVert;
        phongPipelineDesc.vertex.bufferCount = 1;
        phongPipelineDesc.vertex.buffers = &defaultVertexLayout;

        wgpu::ColorTargetState colorTarget;
        colorTarget.format = ctx->getSurfaceFormat();

        wgpu::FragmentState fragmentState = {
            .module = phongFrag,
            .targetCount = 1,
            .targets = &colorTarget
        };
        phongPipelineDesc.fragment = &fragmentState;
        phongPipelineDesc.depthStencil = &defaultDepthStencil;
        phongPipelineDesc.multisample.count = MULTI_SAMPLE_COUNT;

        phongPipeline = ctx->getDevice().CreateRenderPipeline(&phongPipelineDesc);
    }


    void Renderer::initShadowPipeline()
    {
        wgpu::ShaderModule shadowVert = getShaderModule(ctx->getDevice(), "../shaders/full_screen_quad.vert.wgsl",
                                                        "ScreenSpaceShadow vertex shader");
        wgpu::ShaderModule shadowFrag = getShaderModule(ctx->getDevice(),
                                                        "../shaders/screen_space_shadows.frag.wgsl",
                                                        "ScreenSpaceShadow frag shader");

        wgpu::BindGroupLayoutEntry shadowLayoutEntry[3] = {
            {
                .binding = 0,
                .visibility = wgpu::ShaderStage::Fragment,
                .texture = {
                    .sampleType = wgpu::TextureSampleType::Depth,
                    .viewDimension = wgpu::TextureViewDimension::e2D,
                    .multisampled = USE_MULTI_SAMPLE
                }
            },
            {
                .binding = 1,
                .visibility = wgpu::ShaderStage::Fragment,
                .buffer = {
                    .type = wgpu::BufferBindingType::Uniform,
                    .hasDynamicOffset = false,
                    .minBindingSize = sizeof(config->shadowUniform),
                }
            },
            {
                .binding = 2,
                .visibility = wgpu::ShaderStage::Fragment,
                .storageTexture = {
                    .access = wgpu::StorageTextureAccess::WriteOnly,
                    .format = ctx->getSurfaceFormat(),
                    .viewDimension = wgpu::TextureViewDimension::e2D
                }
            }
        };
        wgpu::BindGroupLayoutDescriptor shadowBindGroupLayoutDesc = {
            .label = "Shadow bind group layout",
            .entryCount = 3,
            .entries = &shadowLayoutEntry[0]
        };
        wgpu::BindGroupLayout shadowBindGroupLayout = ctx->getDevice().
                                                           CreateBindGroupLayout(&shadowBindGroupLayoutDesc);

        wgpu::BindGroupLayout bindGroupLayouts[2] = {
            ctx->getDevice().CreateBindGroupLayout(&globalBindGroupLayoutDesc),
            shadowBindGroupLayout
        };
        wgpu::PipelineLayoutDescriptor pipelineLayoutDesc = {
            .label = "Shadow pipeline layout",
            .bindGroupLayoutCount = 2,
            .bindGroupLayouts = &bindGroupLayouts[0]
        };
        wgpu::PipelineLayout pipelineLayout = ctx->getDevice().CreatePipelineLayout(&pipelineLayoutDesc);

        wgpu::RenderPipelineDescriptor shadowPipelineDesc;
        shadowPipelineDesc.label = "Shadow pipeline";
        shadowPipelineDesc.layout = pipelineLayout;
        shadowPipelineDesc.vertex.module = shadowVert;
        shadowPipelineDesc.vertex.bufferCount = 1;
        shadowPipelineDesc.vertex.buffers = &defaultVertexLayout;

        wgpu::BlendState blendState = {
            .color = {
                .operation = wgpu::BlendOperation::Add,
                .srcFactor = wgpu::BlendFactor::SrcAlpha,
                .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha,
            }
        };
        wgpu::ColorTargetState colorTarget = {
            .format = ctx->getSurfaceFormat(),
            .blend = &blendState
        };
        wgpu::FragmentState fragmentState = {
            .module = shadowFrag,
            .targetCount = 1,
            .targets = &colorTarget
        };
        shadowPipelineDesc.fragment = &fragmentState;
        shadowPipelineDesc.multisample.count = MULTI_SAMPLE_COUNT;

        shadowPipeline = ctx->getDevice().CreateRenderPipeline(&shadowPipelineDesc);

        wgpu::BindGroupEntry shadowUniformEntry[3] = {
            {
                .binding = 0,
                .textureView = depthView
            },
            {
                .binding = 1,
                .buffer = shadowUniformBuffer
            },
            {
                .binding = 2,
                .textureView = shadowTexture.CreateView(),
            },
        };
        wgpu::BindGroupDescriptor bindGroupDesc = {
            .label = "Shadow uniform bind group",
            .layout = shadowBindGroupLayout,
            .entryCount = 3,
            .entries = &shadowUniformEntry[0]
        };
        shadowUniformBindGroup = ctx->getDevice().CreateBindGroup(&bindGroupDesc);
    }


    void Renderer::createDepthTextureView()
    {
        wgpu::TextureDescriptor depthTextureDesc = {
            .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc |
            wgpu::TextureUsage::TextureBinding,
            .dimension = wgpu::TextureDimension::e2D,
            .size = {size.width, size.height, 1},
            .format = wgpu::TextureFormat::Depth24Plus,
            .mipLevelCount = 1,
            .sampleCount = MULTI_SAMPLE_COUNT,
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


    void Renderer::updateGlobalUniforms(const Camera& camera, float time, uint32_t frameNumber)
    {
        CameraUniformData camUniforms = {
            camera.viewMatrix,
            camera.projMatrix,
            camera.position,
            0.0,
            camera.direction,
            0.0,
            glm::inverse(camera.viewMatrix),
            glm::inverse(camera.projMatrix)
        };
        camUniforms.dir = camera.direction;
        GlobalUniformData globalUniforms = {
            camUniforms,
            config->lightUniform,
            time,
        };
        ctx->getQueue().WriteBuffer(globalUniformBuffer, 0, &globalUniforms, globalUniformBuffer.GetSize());
    }


    void Renderer::updateBladeUniforms()
    {
        ctx->getQueue().WriteBuffer(bladeUniformBuffer, 0, &config->bladeUniform, bladeUniformBuffer.GetSize());
    }


    void Renderer::updateShadowUniforms()
    {
        ctx->getQueue().WriteBuffer(shadowUniformBuffer, 0, &config->shadowUniform, shadowUniformBuffer.GetSize());
    }


    void Renderer::drawSky(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView)
    {
        wgpu::RenderPassColorAttachment fullScreenPassColorAttachment = {
            .view = USE_MULTI_SAMPLE ? multisampleView : targetView,
            .resolveTarget = nullptr,
            .loadOp = wgpu::LoadOp::Clear,
            .storeOp = wgpu::StoreOp::Store,
        };
        wgpu::RenderPassDescriptor fullScreenPassDesc = {
            .colorAttachmentCount = 1,
            .colorAttachments = &fullScreenPassColorAttachment,
        };

        wgpu::RenderPassEncoder fullScreenPass = encoder.BeginRenderPass(&fullScreenPassDesc);
        fullScreenPass.SetBindGroup(0, globalBindGroup, 0, nullptr);
        fullScreenPass.SetPipeline(skyPipeline);
        fullScreenQuad.draw(fullScreenPass, 1);
        fullScreenPass.End();
    }


    void Renderer::drawGrass(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView)
    {
        wgpu::RenderPassColorAttachment renderPassColorAttachment = {
            .view = USE_MULTI_SAMPLE ? multisampleView : targetView,
            .resolveTarget = nullptr,
            .loadOp = wgpu::LoadOp::Load,
            .storeOp = wgpu::StoreOp::Store,
        };
        wgpu::RenderPassDepthStencilAttachment renderPassDepthAttachment = {
            .view = depthView,
            .depthLoadOp = wgpu::LoadOp::Clear,
            .depthStoreOp = wgpu::StoreOp::Store,
            .depthClearValue = 1.0,
        };
        wgpu::RenderPassDescriptor renderPassDesc = {
            .label = "Grass render pass",
            .colorAttachmentCount = 1,
            .colorAttachments = &renderPassColorAttachment,
            .depthStencilAttachment = &renderPassDepthAttachment,
        };
        wgpu::RenderPassColorAttachment fullSreenColorAttachment = {
            .view = USE_MULTI_SAMPLE ? multisampleView : targetView,
            .loadOp = wgpu::LoadOp::Load,
            .storeOp = wgpu::StoreOp::Store,
        };
        wgpu::RenderPassDescriptor fullSreenPassDesc = {
            .label = "Shadow render Pass",
            .colorAttachmentCount = 1,
            .colorAttachments = &fullSreenColorAttachment,
        };

        wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDesc);
        renderPass.SetPipeline(grassPipeline);
        renderPass.SetBindGroup(0, globalBindGroup, 0, nullptr);
        renderPass.SetBindGroup(1, bladeUniformBindGroup, 0, nullptr);
        bladeGeometry.draw(renderPass, config->totalBlades);
        renderPass.End();

        wgpu::RenderPassEncoder fullScreenPass = encoder.BeginRenderPass(&fullSreenPassDesc);
        fullScreenPass.SetPipeline(shadowPipeline);
        fullScreenPass.SetBindGroup(0, globalBindGroup, 0, nullptr);
        fullScreenPass.SetBindGroup(1, shadowUniformBindGroup, 0, nullptr);
        fullScreenQuad.draw(fullScreenPass, 1);
        fullScreenPass.End();
    }


    void Renderer::drawScene(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView,
                             const std::vector<Mesh>& scene)
    {
        wgpu::RenderPassColorAttachment renderPassColorAttachment = {
            .view = USE_MULTI_SAMPLE ? multisampleView : targetView,
            .resolveTarget = USE_MULTI_SAMPLE ? targetView : nullptr,
            .loadOp = wgpu::LoadOp::Load,
            .storeOp = wgpu::StoreOp::Store,
        };
        wgpu::RenderPassDepthStencilAttachment renderPassDepthAttachment = {
            .view = depthView,
            .depthLoadOp = wgpu::LoadOp::Load,
            .depthStoreOp = wgpu::StoreOp::Store,
        };
        wgpu::RenderPassDescriptor renderPassDesc = {
            .label = "Scene render pass",
            .colorAttachmentCount = 1,
            .colorAttachments = &renderPassColorAttachment,
            .depthStencilAttachment = &renderPassDepthAttachment,
        };

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
        pass.SetPipeline(phongPipeline);
        pass.SetBindGroup(0, globalBindGroup, 0, nullptr);
        for (auto mesh : scene)
        {
            pass.SetBindGroup(1, mesh.material.bindGroup, 0, nullptr);
            pass.SetBindGroup(2, mesh.bindGroup, 0, nullptr);
            mesh.draw(pass, 1);
        }
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
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&imGuiRenderPassDesc);

        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass.Get());

        pass.End();
    }


    void Renderer::render(const std::vector<Mesh>& scene, const Camera& camera, float time, uint32_t frameNumber)
    {
        if (!multisampleView)
        {
            wgpu::TextureDescriptor msTextureDesc = {
                .usage = wgpu::TextureUsage::RenderAttachment,
                .size = {size.width, size.height, 1},
                .format = ctx->getSurfaceFormat(),
                .sampleCount = MULTI_SAMPLE_COUNT
            };
            wgpu::Texture multisampleTexture = ctx->getDevice().CreateTexture(&msTextureDesc);
            multisampleView = multisampleTexture.CreateView();
        }

        wgpu::TextureView targetView = ctx->getNextSurfaceTextureView();
        if (!targetView) return;

        wgpu::CommandEncoderDescriptor encoderDesc;
        wgpu::CommandEncoder encoder = ctx->getDevice().CreateCommandEncoder(&encoderDesc);

        updateGlobalUniforms(camera, time, frameNumber);
        drawSky(encoder, targetView);
        drawGrass(encoder, targetView);
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
