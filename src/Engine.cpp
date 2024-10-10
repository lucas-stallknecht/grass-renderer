#include "Engine.h"
#include "Utils.h"

#include <cassert>
#include <iostream>
#include <glm/glm.hpp>

namespace grass
{
    Engine* loadedEngine = nullptr;
    Engine& Engine::Get() { return *loadedEngine; }

    void Engine::init()
    {
        assert(loadedEngine == nullptr);
        loadedEngine = this;

        initWindow();
        initWebgpu();
        configSurface();
        initVertexBuffer();
        initUniformBuffer();
        initPipeline();
        initDepthTextureView();
    }


    void Engine::initWindow()
    {
        assert(glfwInit() && "Could not initialize GLFW!");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Grass renderer", nullptr, nullptr);
        assert(window && "Could not get window from GLFW!");
    }


    void Engine::initWebgpu()
    {
        wgpu::InstanceDescriptor instanceDesc;
        instanceDesc.features.timedWaitAnyEnable = true;
        wgpu::Instance instance = wgpu::CreateInstance(&instanceDesc);
        assert(instance && "Could not initialize WebGPU!");

        surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);
        assert(surface && "Could not get Surface!");

        wgpu::RequestAdapterOptions adapterOpts = {
            .compatibleSurface = surface,
            .powerPreference = wgpu::PowerPreference::HighPerformance,
        };
        wgpu::Adapter chosenAdapter;

        wgpu::Future adapterFuture = instance.RequestAdapter(
            &adapterOpts,
            wgpu::CallbackMode::WaitAnyOnly,
            [](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter,
               const char* message, wgpu::Adapter* userdata)
            {
                if (status != wgpu::RequestAdapterStatus::Success)
                {
                    fprintf(stderr, "Failed to request adapter: %d\n",
                            status);
                    if (message)
                    {
                        fprintf(stderr, "Message: %s\n", message);
                    }
                    return;
                }
                *userdata = std::move(adapter);
            },
            &chosenAdapter
        );
        instance.WaitAny(adapterFuture, UINT64_MAX);
        assert(chosenAdapter && "Could not request the adapter!");


        // Dawn error toggle
        const char* toggles[] = {
            "enable_immediate_error_handling"
        };
        wgpu::DawnTogglesDescriptor dawnTogglesDesc;
        dawnTogglesDesc.disabledToggleCount = 0;
        dawnTogglesDesc.enabledToggleCount = 1;
        dawnTogglesDesc.enabledToggles = toggles;


        wgpu::DeviceDescriptor deviceDesc;
        deviceDesc.label = "Grass renderer device";
        deviceDesc.requiredFeatureCount = 0;
        deviceDesc.requiredLimits = nullptr;
        deviceDesc.nextInChain = &dawnTogglesDesc;
        deviceDesc.SetDeviceLostCallback(
            wgpu::CallbackMode::AllowSpontaneous,
            [](const wgpu::Device& device, wgpu::DeviceLostReason reason, const char* message) {
                fprintf(stderr, "Device lost: reason: %d\n", reason);
                if (message) fprintf(stderr, "message: %s\n\n", message);
            }
        );
        deviceDesc.SetUncapturedErrorCallback(
            [](const wgpu::Device& device, wgpu::ErrorType type, const char* message) {
                fprintf(stderr, "Uncaptured device error: type: %d\n", type);
                if (message) fprintf(stderr, "message: %s\n\n", message);
            }
        );


        wgpu::Future deviceFuture = chosenAdapter.RequestDevice(
            &deviceDesc,
            wgpu::CallbackMode::WaitAnyOnly,
            [](wgpu::RequestDeviceStatus status, wgpu::Device device,
               const char* message, wgpu::Device* userdata)
            {
                if (status != wgpu::RequestDeviceStatus::Success)
                {
                    fprintf(stderr, "Failed to request device: %d\n",
                            status);
                    if (message)
                    {
                        fprintf(stderr, "Message: %s\n", message);
                    }
                    return;
                }
                *userdata = std::move(device);
            },
            &device
        );
        instance.WaitAny(deviceFuture, UINT64_MAX);
        assert(device && "Could not request device!");

        wgpu::SurfaceCapabilities capabilities;
        surface.GetCapabilities(chosenAdapter, &capabilities);
        surfaceFormat = capabilities.formats[0];

        queue = device.GetQueue();
        assert(queue && "Could not get queue from device!");
    }


    void Engine::configSurface()
    {
        wgpu::SurfaceConfiguration config = {
            .device = device,
            .format = surfaceFormat,
            .usage = wgpu::TextureUsage::RenderAttachment,
            .alphaMode = wgpu::CompositeAlphaMode::Auto,
            .width = WIDTH,
            .height = HEIGHT,
            .presentMode = wgpu::PresentMode::Fifo,
        };
        surface.Configure(&config);
    }


    void Engine::initVertexBuffer()
    {
        std::vector<float> vertices = {
            // Base (Square) - 2 triangles (Normal: 0, -1, 0)
            // Triangle 1
            -0.5f, 0.0f, -0.5f,   0.0f, -1.0f, 0.0f,   0.0f, 0.0f, // Bottom-left
             0.5f, 0.0f, -0.5f,   0.0f, -1.0f, 0.0f,   1.0f, 0.0f, // Bottom-right
             0.5f, 0.0f,  0.5f,   0.0f, -1.0f, 0.0f,   1.0f, 1.0f, // Top-right

            // Triangle 2
            -0.5f, 0.0f, -0.5f,   0.0f, -1.0f, 0.0f,   0.0f, 0.0f, // Bottom-left
             0.5f, 0.0f,  0.5f,   0.0f, -1.0f, 0.0f,   1.0f, 1.0f, // Top-right
            -0.5f, 0.0f,  0.5f,   0.0f, -1.0f, 0.0f,   0.0f, 1.0f, // Top-left

            // Sides - 4 triangular faces
            // Front Face (Normal: 0, 0.707f, 0.707f)
            -0.5f, 0.0f,  0.5f,   0.0f,  0.707f,  0.707f,  0.0f, 0.0f, // Bottom-left
             0.5f, 0.0f,  0.5f,   0.0f,  0.707f,  0.707f,  1.0f, 0.0f, // Bottom-right
             0.0f, 1.0f,  0.0f,   0.0f,  0.707f,  0.707f,  0.5f, 1.0f, // Apex

            // Right Face (Normal: 0.707f, 0.707f, 0.0f)
             0.5f, 0.0f,  0.5f,   0.707f,  0.707f,  0.0f,   0.0f, 0.0f, // Bottom-left
             0.5f, 0.0f, -0.5f,   0.707f,  0.707f,  0.0f,   1.0f, 0.0f, // Bottom-right
             0.0f, 1.0f,  0.0f,   0.707f,  0.707f,  0.0f,   0.5f, 1.0f, // Apex

            // Back Face (Normal: 0.0f, 0.707f, -0.707f)
             0.5f, 0.0f, -0.5f,   0.0f,  0.707f, -0.707f,  0.0f, 0.0f, // Bottom-left
            -0.5f, 0.0f, -0.5f,   0.0f,  0.707f, -0.707f,  1.0f, 0.0f, // Bottom-right
             0.0f, 1.0f,  0.0f,   0.0f,  0.707f, -0.707f,  0.5f, 1.0f, // Apex

            // Left Face (Normal: -0.707f, 0.707f, 0.0f)
            -0.5f, 0.0f, -0.5f,  -0.707f,  0.707f,  0.0f,   0.0f, 0.0f, // Bottom-left
            -0.5f, 0.0f,  0.5f,  -0.707f,  0.707f,  0.0f,   1.0f, 0.0f, // Bottom-right
             0.0f, 1.0f,  0.0f,  -0.707f,  0.707f,  0.0f,   0.5f, 1.0f  // Apex
        };
        wgpu::BufferDescriptor bufferDesc {
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
            .size = vertices.size() * sizeof(float),
            .mappedAtCreation = false,
        };
        vertexBuffer = device.CreateBuffer(&bufferDesc);
        queue.WriteBuffer(vertexBuffer, 0, vertices.data(), bufferDesc.size);
    }


    void Engine::initPipeline()
    {
        wgpu::ShaderModule simpleModule = getShaderModule(device, "../shaders/simple_triangle.wgsl", "Triangle Module");
        // Top level pipeline descriptor -> used to create the pipeline
        wgpu::RenderPipelineDescriptor pipelineDesc;


        // --- Uniform binding ---
        // This is basically a entry (binding at a certain location within a group)
        wgpu::BindGroupLayoutEntry entryLayout = {
            .binding = 0,
            .visibility = wgpu::ShaderStage::Vertex
        };
        entryLayout.buffer.type = wgpu::BufferBindingType::Uniform;
        entryLayout.buffer.minBindingSize = sizeof(glm::mat4);

        // This describes the whole group
        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc = {
            .entryCount = 1,
            .entries = &entryLayout
        };
        wgpu::BindGroupLayout bindGroupLayout = device.CreateBindGroupLayout(&bindGroupLayoutDesc);

        // This describe the groupS if there are multiple
        wgpu::PipelineLayoutDescriptor lipelineLayoutDesc = {
            .bindGroupLayoutCount = 1,
            .bindGroupLayouts = &bindGroupLayout
        };
        wgpu::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&lipelineLayoutDesc);
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
        pipelineDesc.vertex.module = simpleModule;
        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = &vertLayout;


        // --- Fragment state ---
        wgpu::ColorTargetState colorTarget;
        colorTarget.format = surfaceFormat;

        wgpu::FragmentState fragmentState = {
            .module = simpleModule,
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

        grassPipeline = device.CreateRenderPipeline(&pipelineDesc);


        // --- Bind group ---
        // The "real" bindings between the buffer and the shader locs, not just the layout
        wgpu::BindGroupEntry entry = {
            .binding = 0,
            .buffer = uniformBuffer,
            .offset = 0,
            .size = sizeof(glm::mat4)
        };
        wgpu::BindGroupDescriptor bindGroupDesc = {
            .layout = bindGroupLayout,
            .entryCount = bindGroupLayoutDesc.entryCount,
            .entries = &entry
        };
        grassBindGroup = device.CreateBindGroup(&bindGroupDesc);
    }


    wgpu::TextureView Engine::getNextSurfaceTextureView()
    {
        wgpu::SurfaceTexture surfaceTexture;
        surface.GetCurrentTexture(&surfaceTexture);

        if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::Success)
        {
            std::cerr << "Failed to get current texture from the surface: " << std::endl;
            return nullptr;
        }

        wgpu::TextureViewDescriptor viewDescriptor = {
            .label = "Surface texture view",
            .format = surfaceFormat,
            .dimension = wgpu::TextureViewDimension::e2D,
            .baseMipLevel = 0,
            .mipLevelCount = 1,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1,
            .aspect = wgpu::TextureAspect::All,
        };
        return surfaceTexture.texture.CreateView(&viewDescriptor);
    }


    void Engine::initUniformBuffer()
    {
        camera.position = glm::vec3{-1.5, 1.5, 1.5};
        camera.updateMatrix();

        wgpu::BufferDescriptor uniformBufferDesc{
            .label = "Camera uniform buffer",
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
            .size = sizeof(glm::mat4),
            .mappedAtCreation = false,
        };

        uniformBuffer = device.CreateBuffer(&uniformBufferDesc);
        queue.WriteBuffer(uniformBuffer, 0, &camera.viewProjMatrix, uniformBufferDesc.size);
    }


    void Engine::initDepthTextureView()
    {
        wgpu::TextureDescriptor depthTextureDesc = {
            .usage = wgpu::TextureUsage::RenderAttachment,
            .dimension = wgpu::TextureDimension::e2D,
            .size = {WIDTH, HEIGHT, 1},
            .format = wgpu::TextureFormat::Depth24Plus,
            .mipLevelCount = 1,
            .sampleCount = 1,
        };
        wgpu::Texture depthTexture = device.CreateTexture(&depthTextureDesc);

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



    void Engine::draw()
    {
        // Get texture view to draw on, from the surface
        wgpu::TextureView targetView = getNextSurfaceTextureView();
        if (!targetView) return;

        // Create command encoder
        wgpu::CommandEncoderDescriptor encoderDesc;
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);

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
        pass.Draw(18, 1, 0, 0);
        pass.End();

        // Create command buffer
        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {
            .label = "Command buffer"
        };
        // Submit the command buffer
        wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);
        queue.Submit(1, &command);

        surface.Present();
        device.Tick();
    }


    void Engine::run()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            draw();
            frameNmber++;
        }
    }


    void Engine::cleanup()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
} // grass
