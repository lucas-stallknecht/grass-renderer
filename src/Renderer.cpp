#include "Renderer.h"
#include "Utils.h"

#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

#include <iostream>

namespace grass
{
    Renderer::Renderer(std::shared_ptr<GlobalConfig> config, const uint16_t width, const uint16_t height)
        : config(std::move(config)), size(wgpu::Extent2D{width, height})
    {
        ctx = GPUContext::getInstance();
    }


    void Renderer::init(const std::string& meshFilePath, const wgpu::Buffer& computeBuffer, const Camera& camera)
    {

        createUniformBuffers(camera);
        createBladeTextures();
        initRenderPipeline(computeBuffer);
        createDepthTextureView();
    }


    void Renderer::createUniformBuffers(const Camera& camera)
    {
        wgpu::BufferDescriptor camUniformBufferDesc = {
            .label = "Camera uniform buffer",
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
            .size = sizeof(CameraUniformData),
            .mappedAtCreation = false,
        };
        camUniformBuffer = ctx->getDevice().CreateBuffer(&camUniformBufferDesc);

        wgpu::BufferDescriptor bladeDynamicUniformBufferDesc = {
            .label = "Blade : dynamic uniform buffer",
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
            .size = sizeof(BladeDynamicUniformData),
            .mappedAtCreation = false,
        };
        bladeDynamicUniformBuffer = ctx->getDevice().CreateBuffer(&bladeDynamicUniformBufferDesc);

        wgpu::BufferDescriptor bladeStaticUniformBufferDesc = {
            .label = "Blade : static uniform buffer",
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
            .size = sizeof(BladeStaticUniformData),
            .mappedAtCreation = false,
        };
        bladeStaticUniformBuffer = ctx->getDevice().CreateBuffer(&bladeStaticUniformBufferDesc);
    }


    void Renderer::createBladeTextures()
    {
        normalTexture = loadTexture("../assets/blade_normal.png", ctx->getDevice(), ctx->getQueue());

        wgpu::SamplerDescriptor samplerDesc = {};
        textureSampler = ctx->getDevice().CreateSampler(&samplerDesc);
    }


    void Renderer::initRenderPipeline(const wgpu::Buffer& computeBuffer)
    {
        wgpu::ShaderModule vertModule = getShaderModule(ctx->getDevice(), "../shaders/blade.vert.wgsl", "Vertex Module");
        wgpu::ShaderModule fragModule = getShaderModule(ctx->getDevice(), "../shaders/blade.frag.wgsl", "Frag Module");
        // Top level pipeline descriptor -> used to create the pipeline
        wgpu::RenderPipelineDescriptor pipelineDesc;


        // --- Uniform and storage Bindings ---

        // Global uniforms bind group layout (Cam + others globals)
        wgpu::BindGroupLayoutEntry globalEntryLayouts[5] = {
            {
                .binding = 0,
                .visibility = wgpu::ShaderStage::Vertex |  wgpu::ShaderStage::Fragment,
                .buffer = {
                    .type = wgpu::BufferBindingType::Uniform,
                    .minBindingSize = camUniformBuffer.GetSize()
                }
            },
            {
                .binding = 1,
                .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
                .buffer = {
                    .type = wgpu::BufferBindingType::Uniform,
                    .minBindingSize = bladeDynamicUniformBuffer.GetSize()
                }
            },
            {
                .binding = 2,
                .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
                .buffer = {
                    .type = wgpu::BufferBindingType::Uniform,
                    .minBindingSize = bladeStaticUniformBuffer.GetSize()
                }
            },
            {
                .binding = 3,
                .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
                .texture = {
                    .sampleType = wgpu::TextureSampleType::Float,
                    .viewDimension = wgpu::TextureViewDimension::e2D
                },
            },
            {
                .binding = 4,
                .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
                .sampler = {
                    .type = wgpu::SamplerBindingType::Filtering,
                }
            }
        };
        wgpu::BindGroupLayoutDescriptor globalBindGroupLayoutDesc = {
            .label = "Global uniforms bind group layout",
            .entryCount = 5,
            .entries = &globalEntryLayouts[0]
        };
        wgpu::BindGroupLayout globalBindGroupLayout = ctx->getDevice().CreateBindGroupLayout(&globalBindGroupLayoutDesc);

        // SSBO bind group layout (compute buffer)
        wgpu::BindGroupLayoutEntry storageEntryLayouts[2] = {
            {
                .binding = 0,
                .visibility = wgpu::ShaderStage::Vertex,
                .buffer = {
                    .type = wgpu::BufferBindingType::ReadOnlyStorage,
                    .minBindingSize = computeBuffer.GetSize()
                }
            }
        };
        wgpu::BindGroupLayoutDescriptor storageBindGroupLayoutDesc = {
            .label = "Storage bind group layout",
            .entryCount = 1,
            .entries = &storageEntryLayouts[0]
        };
        wgpu::BindGroupLayout storageBindGroupLayout = ctx->getDevice().CreateBindGroupLayout(&storageBindGroupLayoutDesc);


        wgpu::BindGroupLayout bindGroupLayouts[2] = {globalBindGroupLayout, storageBindGroupLayout};
        wgpu::PipelineLayoutDescriptor lipelineLayoutDesc = {
            .bindGroupLayoutCount = 2,
            .bindGroupLayouts = &bindGroupLayouts[0]
        };
        wgpu::PipelineLayout pipelineLayout = ctx->getDevice().CreatePipelineLayout(&lipelineLayoutDesc);
        pipelineDesc.layout = pipelineLayout;


        // --- Attribute binding ---
        wgpu::VertexAttribute vertAttrib[3] = {
            {
                .format = wgpu::VertexFormat::Float32x3,
                .offset = 0,
                .shaderLocation = 0,
            },
            {
                .format = wgpu::VertexFormat::Float32x3,
                .offset = 3 * sizeof(float),
                .shaderLocation = 1,
            },
            {
                .format = wgpu::VertexFormat::Float32x2,
                .offset = 6 * sizeof(float),
                .shaderLocation = 2,
            }
        };
        wgpu::VertexBufferLayout vertLayout = {
            .arrayStride = 8 * sizeof(float),
            .stepMode = wgpu::VertexStepMode::Vertex,
            .attributeCount = 3,
            // this must target the first element of the array if there are multiple attributes
            .attributes = &vertAttrib[0]
        };
        pipelineDesc.vertex.module = vertModule;
        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = &vertLayout;


        // --- Fragment state ---
        wgpu::ColorTargetState colorTarget;
        colorTarget.format = ctx->getSurfaceFormat();

        wgpu::FragmentState fragmentState = {
            .module = fragModule,
            .targetCount = 1,
            .targets = &colorTarget
        };
        pipelineDesc.fragment = &fragmentState;

        // --- Depth  ---
        wgpu::DepthStencilState depthStencil = {
            .format = wgpu::TextureFormat::Depth24Plus,
            .depthWriteEnabled = true,
            .depthCompare = wgpu::CompareFunction::Less,
            .stencilReadMask = 0,
            .stencilWriteMask = 0
        };
        pipelineDesc.depthStencil = &depthStencil;

        pipelineDesc.label = "Blade of grass pipeline";
        grassPipeline = ctx->getDevice().CreateRenderPipeline(&pipelineDesc);

        // TEMPORARY
        wgpu::TextureViewDescriptor textureViewDesc = {
            .format = normalTexture.GetFormat(),
            .dimension = wgpu::TextureViewDimension::e2D,
            .mipLevelCount = 1,
            .arrayLayerCount = 1
        };
        wgpu::TextureView textureView = normalTexture.CreateView(&textureViewDesc);


        // --- Bind group ---
        // The "real" bindings between the buffer and the shader locs, not just the layout
        wgpu::BindGroupEntry globalEntries[5] = {
            {
                .binding = 0,
                .buffer = camUniformBuffer,
                .offset = 0,
                .size = camUniformBuffer.GetSize()
            },
            {
                .binding = 1,
                .buffer = bladeDynamicUniformBuffer,
                .offset = 0,
                .size = bladeDynamicUniformBuffer.GetSize()
            },
            {
                .binding = 2,
                .buffer = bladeStaticUniformBuffer,
                .offset = 0,
                .size = bladeStaticUniformBuffer.GetSize()
            },
            {
                .binding = 3,
                .textureView = textureView,
            },
            {
                .binding = 4,
                .sampler = textureSampler,
            }
        };
        wgpu::BindGroupDescriptor globalBindGroupDesc = {
            .label = "Global uniforms bind group",
            .layout = globalBindGroupLayout,
            .entryCount = globalBindGroupLayoutDesc.entryCount,
            .entries = &globalEntries[0]
        };
        globalBindGroup = ctx->getDevice().CreateBindGroup(&globalBindGroupDesc);

        wgpu::BindGroupEntry storageEntries[2] = {
            {
                .binding = 0,
                .buffer = computeBuffer,
                .offset = 0,
                .size = computeBuffer.GetSize()
            },
        };
        wgpu::BindGroupDescriptor storageBindGroupDesc = {
            .label = "Storage bind group",
            .layout = storageBindGroupLayout,
            .entryCount = storageBindGroupLayoutDesc.entryCount,
            .entries = &storageEntries[0]
        };
        storageBindGroup = ctx->getDevice().CreateBindGroup(&storageBindGroupDesc);
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


    void Renderer::updateDynamicUniforms(const Camera& camera, float time)
    {
        CameraUniformData camUniforms = {
            camera.viewMatrix,
            camera.projMatrix,
            camera.position,
        };
        camUniforms.dir = camera.direction;
        ctx->getQueue().WriteBuffer(camUniformBuffer, 0, &camUniforms, camUniformBuffer.GetSize());

        ctx->getQueue().WriteBuffer(bladeDynamicUniformBuffer, 0, &time, bladeDynamicUniformBuffer.GetSize());
    }


    void Renderer::updateStaticUniforms(const BladeStaticUniformData& uniform)
    {
        ctx->getQueue().WriteBuffer(bladeStaticUniformBuffer, 0, &uniform, bladeStaticUniformBuffer.GetSize());
    }

    void Renderer::drawScene(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView, Mesh& mesh)
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
        pass.SetPipeline(grassPipeline);
        pass.SetBindGroup(0, globalBindGroup, 0, nullptr);
        pass.SetBindGroup(1, storageBindGroup, 0, nullptr);
        mesh.draw(pass, config->totalBlades);
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



    void Renderer::draw(Mesh& mesh)
    {
        wgpu::TextureView targetView = ctx->getNextSurfaceTextureView();
        if (!targetView) return;

        wgpu::CommandEncoderDescriptor encoderDesc;
        wgpu::CommandEncoder encoder = ctx->getDevice().CreateCommandEncoder(&encoderDesc);

        drawScene(encoder, targetView, mesh);
        drawGUI(encoder, targetView);

        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {
            .label = "Rendering operations command buffer"
        };
        wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);
        ctx->getQueue().Submit(1, &command);
        ctx->getSurface().Present();
    }
} // grass
