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
        initRenderPipeline();
        initDepthTextureView();
        initComputeBuffer();
        initComputPipeline();
        compute();
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
        instance = wgpu::CreateInstance(&instanceDesc);
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
        std::vector<VertexData> verticesData;
        if(!loadMesh("../assets/grass_blade.obj", verticesData))
        {
            std::cerr << "Could not load geometry!" << std::endl;
        }

        vertexCount = verticesData.size();
        wgpu::BufferDescriptor bufferDesc {
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
            .size = verticesData.size() * sizeof(VertexData),
            .mappedAtCreation = false,
        };
        vertexBuffer = device.CreateBuffer(&bufferDesc);
        queue.WriteBuffer(vertexBuffer, 0, verticesData.data(), bufferDesc.size);
    }


    void Engine::initRenderPipeline()
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


    void Engine::initComputeBuffer()
    {
        wgpu::BufferDescriptor bufferDesc = {
            .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc,
            .size = sizeof(glm::vec3),
            .mappedAtCreation = false
        };

        testComputeBuffer = device.CreateBuffer(&bufferDesc);

        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
        readableColorTestBuffer = device.CreateBuffer(&bufferDesc);
    }


    void Engine::initComputPipeline()
    {
        wgpu::ShaderModule computeModule = getShaderModule(device, "../shaders/grass_position.compute.wgsl", "Grass position compute module");
        wgpu::ComputePipelineDescriptor computePipelineDesc;

        wgpu::ProgrammableStageDescriptor computeStageDesc = {
            .module = computeModule,
            .entryPoint = "main"
        };
        computePipelineDesc.compute = computeStageDesc;


        wgpu::BindGroupLayoutEntry entryLayout = {
            .binding = 0,
            .visibility = wgpu::ShaderStage::Compute,
        };
        entryLayout.buffer.type = wgpu::BufferBindingType::Storage;
        entryLayout.buffer.minBindingSize = sizeof(glm::vec3);

        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc = {
            .entryCount = 1,
            .entries = &entryLayout
        };
        wgpu::BindGroupLayout bindGroupLayout = device.CreateBindGroupLayout(&bindGroupLayoutDesc);

        wgpu::PipelineLayoutDescriptor pipelineLayoutDesc = {
            .bindGroupLayoutCount = 1,
            .bindGroupLayouts = &bindGroupLayout
        };
        wgpu::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&pipelineLayoutDesc);
        computePipelineDesc.layout = pipelineLayout;

        computePipeline = device.CreateComputePipeline(&computePipelineDesc);



        wgpu::BindGroupEntry entry = {
            .binding = 0,
            .buffer = testComputeBuffer,
            .offset = 0,
            .size = testComputeBuffer.GetSize(),
        };

        wgpu::BindGroupDescriptor bindGroupDesc = {
            .layout = bindGroupLayout,
            .entryCount = 1,
            .entries = &entry
        };
        computeBindGroup = device.CreateBindGroup(&bindGroupDesc);
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
        camera.position = glm::vec3{1.0, 0.0, 1.0};
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


    void Engine::compute()
    {
        wgpu::CommandEncoderDescriptor encoderDesc;
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);

        wgpu::ComputePassDescriptor computePassDesc;
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&computePassDesc);

        pass.SetPipeline(computePipeline);
        pass.SetBindGroup(0, computeBindGroup);
        pass.DispatchWorkgroups(1, 1, 1);
        pass.End();
        encoder.CopyBufferToBuffer(testComputeBuffer, 0, readableColorTestBuffer, 0, sizeof(glm::vec3));


        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {
            .label = "Command buffer"
        };
        // Submit the command buffer
        wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);
        queue.Submit(1, &command);

        device.Tick();
    }


    void Engine::draw()
    {
        // Get texture view to draw on, from the surface
        wgpu::TextureView targetView = getNextSurfaceTextureView();
        if (!targetView) return;

        // Create command encoder
        wgpu::CommandEncoderDescriptor encoderDesc;
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);


        if(readableColorTestBuffer.GetMapState() != wgpu::BufferMapState::Mapped)
        {
            wgpu::Future readColorFuture = readableColorTestBuffer.MapAsync(
                       wgpu::MapMode::Read,
                       0,
                       sizeof(glm::vec3),
                       wgpu::CallbackMode::WaitAnyOnly,
                       [](wgpu::MapAsyncStatus status, char const * message)
                       {
                           if (status != wgpu::MapAsyncStatus::Success) return;
                       }
                   );
            instance.WaitAny(readColorFuture, UINT8_MAX);
            const auto color = static_cast<const glm::vec3*>(readableColorTestBuffer.GetConstMappedRange(0, sizeof(glm::vec3)));
            clearColor = *color;
        }

        // Encode render pass
        wgpu::RenderPassColorAttachment renderPassColorAttachment = {
            .view = targetView,
            .resolveTarget = nullptr,
            .loadOp = wgpu::LoadOp::Clear,
            .storeOp = wgpu::StoreOp::Store,
            .clearValue = wgpu::Color{clearColor.r, clearColor.g, clearColor.b, 1.0},
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
        pass.Draw(vertexCount, 1, 0, 0);
        pass.End();

        // Create command buffer
        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {
            .label = "Command buffer"
        };
        // Submit the command buffer
        wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);
        queue.Submit(1, &command);

        device.Tick();
        surface.Present();
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
