#include "Renderer.h"
#include "Utils.h"

#include <iostream>

namespace grass
{
    Renderer::Renderer(std::shared_ptr<wgpu::Device> device, std::shared_ptr<wgpu::Queue> queue,
                       wgpu::TextureFormat surfaceFormat)
        : device(std::move(device)), queue(std::move(queue)), surfaceFormat(surfaceFormat)
    {
    }


    void Renderer::init(const std::string& meshFilePath, const wgpu::Buffer& computeBuffer, const Camera& camera)
    {
        createVertexBuffer(meshFilePath);
        createUniformBuffers(camera);
        createBladeTextures();
        initRenderPipeline(computeBuffer);
        createDepthTextureView();
    }


    void Renderer::createVertexBuffer(const std::string& meshFilePath)
    {
        std::vector<VertexData> verticesData;
        if (!loadMesh(meshFilePath, verticesData))
        {
            std::cerr << "Could not load geometry!" << std::endl;
        }


        vertexCount = verticesData.size();
        wgpu::BufferDescriptor bufferDesc{
            .label = "Blade of grass vertex buffer",
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
            .size = verticesData.size() * sizeof(VertexData),
            .mappedAtCreation = false,
        };
        vertexBuffer = device->CreateBuffer(&bufferDesc);
        queue->WriteBuffer(vertexBuffer, 0, verticesData.data(), bufferDesc.size);
    }


    void Renderer::createUniformBuffers(const Camera& camera)
    {
        wgpu::BufferDescriptor camUniformBufferDesc = {
            .label = "Camera uniform buffer",
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
            .size = sizeof(CameraUniform),
            .mappedAtCreation = false,
        };
        camUniformBuffer = device->CreateBuffer(&camUniformBufferDesc);

        wgpu::BufferDescriptor settingsUniformBufferDesc = {
            .label = "Settings uniform buffer",
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
            .size = sizeof(GrassVertexSettingsUniforms),
            .mappedAtCreation = false,
        };
        settingsUniformBuffer = device->CreateBuffer(&settingsUniformBufferDesc);
    }


    void Renderer::createBladeTextures()
    {
        normalTexture = loadTexture("../assets/blade_normal.png", *device, *queue);

        wgpu::SamplerDescriptor samplerDesc = {};
        textureSampler = device->CreateSampler(&samplerDesc);
    }


    void Renderer::initRenderPipeline(const wgpu::Buffer& computeBuffer)
    {
        wgpu::ShaderModule vertModule = getShaderModule(*device, "../shaders/blade.vert.wgsl", "Vertex Module");
        wgpu::ShaderModule fragModule = getShaderModule(*device, "../shaders/blade.frag.wgsl", "Frag Module");
        // Top level pipeline descriptor -> used to create the pipeline
        wgpu::RenderPipelineDescriptor pipelineDesc;


        // --- Uniform and storage Bindings ---

        // Global uniforms bind group layout (Cam + others globals)
        wgpu::BindGroupLayoutEntry globalEntryLayouts[4] = {
            {
                .binding = 0,
                .visibility = wgpu::ShaderStage::Vertex,
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
                    .minBindingSize = settingsUniformBuffer.GetSize()
                }
            },
            {
                .binding = 2,
                .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
                .texture = {
                    .sampleType = wgpu::TextureSampleType::Float,
                    .viewDimension = wgpu::TextureViewDimension::e2D
                },
            },
            {
                .binding = 3,
                .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
                .sampler = {
                    .type = wgpu::SamplerBindingType::Filtering,
                }
            }
        };
        wgpu::BindGroupLayoutDescriptor globalBindGroupLayoutDesc = {
            .label = "Global uniforms bind group layout",
            .entryCount = 4,
            .entries = &globalEntryLayouts[0]
        };
        wgpu::BindGroupLayout globalBindGroupLayout = device->CreateBindGroupLayout(&globalBindGroupLayoutDesc);

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
        wgpu::BindGroupLayout storageBindGroupLayout = device->CreateBindGroupLayout(&storageBindGroupLayoutDesc);


        wgpu::BindGroupLayout bindGroupLayouts[2] = {globalBindGroupLayout, storageBindGroupLayout};
        wgpu::PipelineLayoutDescriptor lipelineLayoutDesc = {
            .bindGroupLayoutCount = 2,
            .bindGroupLayouts = &bindGroupLayouts[0]
        };
        wgpu::PipelineLayout pipelineLayout = device->CreatePipelineLayout(&lipelineLayoutDesc);
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
        colorTarget.format = surfaceFormat;

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
        grassPipeline = device->CreateRenderPipeline(&pipelineDesc);

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
        wgpu::BindGroupEntry globalEntries[4] = {
            {
                .binding = 0,
                .buffer = camUniformBuffer,
                .offset = 0,
                .size = camUniformBuffer.GetSize()
            },
            {
                .binding = 1,
                .buffer = settingsUniformBuffer,
                .offset = 0,
                .size = settingsUniformBuffer.GetSize()
            },
            {
                .binding = 2,
                .textureView = textureView,
            },
            {
                .binding = 3,
                .sampler = textureSampler,
            }
        };
        wgpu::BindGroupDescriptor globalBindGroupDesc = {
            .label = "Global uniforms bind group",
            .layout = globalBindGroupLayout,
            .entryCount = globalBindGroupLayoutDesc.entryCount,
            .entries = &globalEntries[0]
        };
        globalBindGroup = device->CreateBindGroup(&globalBindGroupDesc);

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
        storageBindGroup = device->CreateBindGroup(&storageBindGroupDesc);
    }


    void Renderer::createDepthTextureView()
    {
        wgpu::TextureDescriptor depthTextureDesc = {
            .usage = wgpu::TextureUsage::RenderAttachment,
            .dimension = wgpu::TextureDimension::e2D,
            .size = {WIDTH, HEIGHT, 1},
            .format = wgpu::TextureFormat::Depth24Plus,
            .mipLevelCount = 1,
            .sampleCount = 1,
        };
        wgpu::Texture depthTexture = device->CreateTexture(&depthTextureDesc);

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


    void Renderer::updateUniforms(const Camera& camera, GrassVertexSettingsUniforms& settingsUniforms,
                                  float_t time)
    {
        CameraUniform camUniforms = {
            camera.viewMatrix,
            camera.projMatrix,
            camera.position,
        };
        camUniforms.dir = camera.direction;
        queue->WriteBuffer(camUniformBuffer, 0, &camUniforms, camUniformBuffer.GetSize());

        settingsUniforms.time = time;
        queue->WriteBuffer(settingsUniformBuffer, 0, &settingsUniforms, settingsUniformBuffer.GetSize());
    }


    void Renderer::draw(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView,
                        const GrassGenerationSettings& grassSettings)
    {
        // Encode render pass
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
        pass.SetVertexBuffer(0, vertexBuffer, 0, vertexBuffer.GetSize());
        pass.Draw(vertexCount, grassSettings.totalBlades, 0, 0);
        pass.End();
    }
} // grass
