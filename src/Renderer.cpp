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
        createUniformBuffer(camera);
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


    void Renderer::createUniformBuffer(const Camera& camera)
    {
        wgpu::BufferDescriptor uniformBufferDesc = {
            .label = "Camera uniform buffer",
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
            .size = sizeof(CameraUniform),
            .mappedAtCreation = false,
        };

        uniformBuffer = device->CreateBuffer(&uniformBufferDesc);

        CameraUniform camUniform = {
            camera.viewMatrix,
            camera.projMatrix,
            camera.position,
        };
        camUniform.dir = camera.direction;
        queue->WriteBuffer(uniformBuffer, 0, &camUniform, uniformBufferDesc.size);
    }


    void Renderer::initRenderPipeline(const wgpu::Buffer& computeBuffer)
    {
        wgpu::ShaderModule vertModule = getShaderModule(*device, "../shaders/blade.vert.wgsl", "Vertex Module");
        wgpu::ShaderModule fragModule = getShaderModule(*device, "../shaders/blade.frag.wgsl", "Frag Module");
        // Top level pipeline descriptor -> used to create the pipeline
        wgpu::RenderPipelineDescriptor pipelineDesc;


        // --- Uniform binding ---
        // This is basically a entry (binding at a certain location within a group)
        wgpu::BindGroupLayoutEntry entryLayouts[2] = {
            {
                .binding = 0,
                .visibility = wgpu::ShaderStage::Vertex,
                .buffer = {
                    .type = wgpu::BufferBindingType::Uniform,
                    .minBindingSize = sizeof(CameraUniform)
                }
            },
            {
                .binding = 1,
                .visibility = wgpu::ShaderStage::Vertex,
                .buffer = {
                    .type = wgpu::BufferBindingType::ReadOnlyStorage,
                    .minBindingSize = computeBuffer.GetSize()
                }
            }
        };

        // This describes the whole group
        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc = {
            .entryCount = 2,
            .entries = &entryLayouts[0]
        };
        wgpu::BindGroupLayout bindGroupLayout = device->CreateBindGroupLayout(&bindGroupLayoutDesc);

        // This describe the groupS if there are multiple
        wgpu::PipelineLayoutDescriptor lipelineLayoutDesc = {
            .bindGroupLayoutCount = 1,
            .bindGroupLayouts = &bindGroupLayout
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


        // --- Bind group ---
        // The "real" bindings between the buffer and the shader locs, not just the layout
        wgpu::BindGroupEntry entries[2] = {
            {
                .binding = 0,
                .buffer = uniformBuffer,
                .offset = 0,
                .size = sizeof(CameraUniform)
            },
            {
                .binding = 1,
                .buffer = computeBuffer,
                .offset = 0,
                .size = computeBuffer.GetSize()
            }
        };
        wgpu::BindGroupDescriptor bindGroupDesc = {
            .label = "Blade of grass bind group",
            .layout = bindGroupLayout,
            .entryCount = bindGroupLayoutDesc.entryCount,
            .entries = &entries[0]
        };
        grassBindGroup = device->CreateBindGroup(&bindGroupDesc);
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


    void Renderer::updateUniforms(const Camera& camera)
    {
        CameraUniform camUniform = {
            camera.viewMatrix,
            camera.projMatrix,
            camera.position,
        };
        camUniform.dir = camera.direction;
        queue->WriteBuffer(uniformBuffer, 0, &camUniform, uniformBuffer.GetSize());
    }


    void Renderer::draw(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView,
                        const GrassSettings& grassSettings)
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
        pass.SetBindGroup(0, grassBindGroup, 0, nullptr);
        pass.SetVertexBuffer(0, vertexBuffer, 0, vertexBuffer.GetSize());
        pass.Draw(vertexCount, grassSettings.totalBlades, 0, 0);
        pass.End();
    }
} // grass
