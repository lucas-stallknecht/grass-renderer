#include "Engine.h"
#include "Utils.h"

#include <cassert>
#include <iostream>

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
        createTestBuffer();
        createPipeline();
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


    void Engine::createTestBuffer()
    {
        std::vector<float> vertices = {
            -0.5, -0.5,
            0.5, -0.5,
            0.0, 0.5
        };
        wgpu::BufferDescriptor bufferDesc {
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
            .mappedAtCreation = false,
        };
        bufferDesc.size = vertices.size() * sizeof(float);
        testBuffer = device.CreateBuffer(&bufferDesc);
        queue.WriteBuffer(testBuffer, 0, vertices.data(), bufferDesc.size);
    }


    void Engine::createPipeline()
    {
        wgpu::ShaderModule simpleModule = getShaderModule(device, "../shaders/simple_triangle.wgsl", "Triangle Module");


        wgpu::VertexAttribute attrib{
            .format = wgpu::VertexFormat::Float32x2,
            .shaderLocation = 0,
        };
        attrib.offset = 0;
        wgpu::VertexBufferLayout vertLayout;
        vertLayout.stepMode = wgpu::VertexStepMode::Vertex;
        vertLayout.arrayStride = 2 * sizeof(float);
        vertLayout.attributeCount = 1;
        vertLayout.attributes = &attrib;

        wgpu::RenderPipelineDescriptor pipelineDesc;
        pipelineDesc.vertex.module = simpleModule;
        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = &vertLayout;

        wgpu::ColorTargetState colorTarget;
        colorTarget.format = surfaceFormat;

        wgpu::FragmentState fragmentState;
        fragmentState.module = simpleModule;
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;
        pipelineDesc.fragment = &fragmentState;

        pipelineDesc.depthStencil = nullptr;

        simplePipeline = device.CreateRenderPipeline(&pipelineDesc);
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


    void Engine::draw()
    {
        // Get texture view to draw on, from the surface
        wgpu::TextureView targetView = getNextSurfaceTextureView();
        if (!targetView) return;


        // Create command encoder
        wgpu::CommandEncoderDescriptor encoderDesc = {};
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);


        // Encode render pass
        wgpu::RenderPassColorAttachment renderPassColorAttachment = {
            .view = targetView,
            .resolveTarget = nullptr,
            .loadOp = wgpu::LoadOp::Clear,
            .storeOp = wgpu::StoreOp::Store,
            .clearValue = wgpu::Color{sin(frameNmber / 120.0), 0.1, 0.2, 1.0},
        };

        wgpu::RenderPassDescriptor renderPassDesc = {
            .colorAttachmentCount = 1,
            .colorAttachments = &renderPassColorAttachment,
        };
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
        pass.SetPipeline(simplePipeline);
        pass.SetVertexBuffer(0, testBuffer, 0, testBuffer.GetSize());
        pass.Draw(3, 1, 0, 0);
        pass.End();


        // Create command buffer
        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {
            .label = "Command buffer",
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
