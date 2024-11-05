#include "Engine.h"
#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "Utils.h"

#include <cassert>
#include <iostream>
#include <glm/glm.hpp>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

namespace grass
{
    Engine* loadedEngine = nullptr;
    Engine& Engine::Get() { return *loadedEngine; }


    void Engine::init()
    {
        assert(loadedEngine == nullptr);
        loadedEngine = this;

        keysArePressed = new bool[512]{false};
        ctx = std::make_shared<Context>();

        initWindow();
        initWebgpu();
        configSurface();

        computeManager = std::make_unique<ComputeManager>(ctx);
        wgpu::Buffer computeBuffer = computeManager->init();

        renderer = std::make_unique<Renderer>(ctx, WIDTH, HEIGHT);
        renderer->init("../assets/grass_blade.obj", computeBuffer, camera);

        initGUI();
    }


    void Engine::initWindow()
    {
        assert(glfwInit() && "Could not initialize GLFW!");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Grass renderer", nullptr, nullptr);
        assert(window && "Could not get window from GLFW!");

        glfwSetWindowUserPointer(window, this);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        auto keyCallback = [](GLFWwindow *window, int key, int scancode, int action, int mods) {
            auto *engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));
            engine->keyCallback(window, key);
        };
        auto mouseCallback = [](GLFWwindow *window, double xpos, double ypos) {
            auto *engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));
            engine->mouseCallback(window, static_cast<float>(xpos), static_cast<float>(ypos));
        };
        auto mouseButtonCallback = [](GLFWwindow *window, int button, int action, int mods) {
            auto *engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));
            engine->mouseButtonCallback(window, button, action);
        };
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
    }


    void Engine::initGUI()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        io = &ImGui::GetIO();
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOther(window, true);
        ImGui_ImplWGPU_InitInfo info = {};
        info.Device = ctx->device.Get();
        info.NumFramesInFlight = 3;
        info.RenderTargetFormat = static_cast<WGPUTextureFormat>(ctx->surfaceFormat);
        ImGui_ImplWGPU_Init(&info);
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
            [](const wgpu::Device& device, wgpu::DeviceLostReason reason, const char* message)
            {
                fprintf(stderr, "Device lost: reason: %d\n", reason);
                if (message) fprintf(stderr, "message: %s\n\n", message);
            }
        );
        deviceDesc.SetUncapturedErrorCallback(
            [](const wgpu::Device& device, wgpu::ErrorType type, const char* message)
            {
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
            &ctx->device
        );
        instance.WaitAny(deviceFuture, UINT64_MAX);
        assert(ctx->device && "Could not request device!");

        wgpu::SurfaceCapabilities capabilities;
        surface.GetCapabilities(chosenAdapter, &capabilities);
        ctx->surfaceFormat = capabilities.formats[0];
        assert(ctx->surfaceFormat != wgpu::TextureFormat::Undefined && "Wrong surface format!");

        ctx->queue = ctx->device.GetQueue();
        assert(ctx->queue && "Could not get queue from device!");
        
    }


    void Engine::configSurface()
    {
        wgpu::SurfaceConfiguration config = {
            .device = ctx->device,
            .format = ctx->surfaceFormat,
            .usage = wgpu::TextureUsage::RenderAttachment,
            .alphaMode = wgpu::CompositeAlphaMode::Auto,
            .width = WIDTH,
            .height = HEIGHT,
            .presentMode = wgpu::PresentMode::Fifo,
        };
        surface.Configure(&config);
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
            .format = ctx->surfaceFormat,
            .dimension = wgpu::TextureViewDimension::e2D,
            .baseMipLevel = 0,
            .mipLevelCount = 1,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1,
            .aspect = wgpu::TextureAspect::All,
        };
        return surfaceTexture.texture.CreateView(&viewDescriptor);
    }


    void Engine::keyCallback(GLFWwindow *window, int key) {
        keysArePressed[key] = (glfwGetKey(window, key) == GLFW_PRESS);
    }


    void Engine::mouseCallback(GLFWwindow *window, float xpos, float ypos) {

        // is right-clicked basically
        if (!focused)
            return;

        // the mouse was not focused the frame before
        if (firstMouse) {
            lastMousePosition.x = xpos;
            lastMousePosition.y = ypos;
            firstMouse = false;
        }
        float xOffset = xpos - lastMousePosition.x;
        float yOffset = lastMousePosition.y - ypos;

        lastMousePosition.x = xpos;
        lastMousePosition.y = ypos;

        camera.updateCamDirection(xOffset, yOffset);
    }


    void Engine::mouseButtonCallback(GLFWwindow *window, int button, int action) {
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
            focused = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
            focused = false;
            firstMouse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

    }


    void Engine::keyInput() {
        float deltaTime = io->DeltaTime;

        if (keysArePressed['W'] && focused) {
            camera.moveForward(deltaTime);
        }
        if (keysArePressed['S'] && focused) {
            camera.moveBackward(deltaTime);
        }
        if (keysArePressed['A'] && focused) {
            camera.moveLeft(deltaTime);
        }
        if (keysArePressed['D'] && focused) {
            camera.moveRight(deltaTime);
        }
        if (keysArePressed['Q'] && focused) {
            camera.moveDown(deltaTime);
        }
        if (keysArePressed[' '] && focused) {
            camera.moveUp(deltaTime);
        }
    }


    void Engine::updateGUI(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView)
    {
        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::Begin("Generation settings");
            ImGui::Text("Number of blades : %i", ctx->totalBlades);
            bool genChange = false;
            if (ImGui::CollapsingHeader("Field", ImGuiTreeNodeFlags_DefaultOpen)) {
                genChange |= ImGui::SliderFloat("Size noise scale", &ctx->grassUniform.sizeNoiseFrequency, 0.02, 0.7, "%.2f");
            }
            if (ImGui::CollapsingHeader("Height", ImGuiTreeNodeFlags_DefaultOpen)) {
                genChange |= ImGui::SliderFloat("Base", &ctx->grassUniform.bladeHeight, 0.1, 2.0, "%.1f");
                genChange |= ImGui::SliderFloat("Delta", &ctx->grassUniform.sizeNoiseAmplitude, 0.05, 0.60, "%.2f");
            }
            if (genChange) {
                computeManager->generate();
            }
            ImGui::End();

            ImGui::Begin("Wind and movement settings");
            bool movChange = false;
            if (ImGui::CollapsingHeader("Wind", ImGuiTreeNodeFlags_DefaultOpen)) {
                movChange |= ImGui::SliderFloat3("Direction (don't touch y)", &ctx->movUniform.wind.r, -1.0, 1.0, "%.1f");
                movChange |= ImGui::SliderFloat("Strength", &ctx->movUniform.wind.w, 0.01, 1.0, "%.2f");
                movChange |= ImGui::SliderFloat("Frequency", &ctx->movUniform.windFrequency, 0.01, 2.0, "%.2f");
            }
            if (movChange) {
                computeManager->updateMovSettingsUniorm();
            }
            ImGui::End();


            ImGui::Begin("Blades settings");
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);

            bool bladeChange = false;
            if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                bladeChange |= ImGui::SliderFloat3("Direction", &ctx->bladeUniform.lightDirection.r, -1.0, 1.0, "%.1f");
                bladeChange |= ImGui::ColorEdit3("Color", &ctx->bladeUniform.lightCol.r, 0);
            }
            if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
                bladeChange |= ImGui::ColorEdit3("Blade Color", &ctx->bladeUniform.bladeCol.r, 0);
                bladeChange |= ImGui::ColorEdit3("Specular Color", &ctx->bladeUniform.specularCol.r, 0);
                bladeChange |= ImGui::SliderFloat("Ambient", &ctx->bladeUniform.ambientStrength, 0.0, 1.0, "%.2f");
                bladeChange |= ImGui::SliderFloat("Diffuse", &ctx->bladeUniform.diffuseStrength, 0.0, 1.0, "%.2f");
                bladeChange |= ImGui::SliderFloat("Wrap value", &ctx->bladeUniform.wrapValue, 0.0, 1.0, "%.2f");
                bladeChange |= ImGui::SliderFloat("Specular", &ctx->bladeUniform.specularStrength, 0.0, 0.3, "%.3f");
            }
            if (bladeChange) {
                renderer->updateStaticUniforms(ctx->bladeUniform);
            }
            ImGui::End();
        }
        ImGui::EndFrame();
        ImGui::Render();

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



    void Engine::run()
    {
        computeManager->generate();
        computeManager->updateMovSettingsUniorm();
        renderer->updateStaticUniforms(ctx->bladeUniform);

        while (!glfwWindowShouldClose(window))
        {
            time += io->DeltaTime;

            keyInput();
            glfwPollEvents();
            camera.updateMatrix();
            renderer->updateDynamicUniforms(camera, time);

            wgpu::CommandEncoderDescriptor encoderDesc;
            wgpu::CommandEncoder encoder = ctx->device.CreateCommandEncoder(&encoderDesc);

            wgpu::TextureView targetView = getNextSurfaceTextureView();
            if (!targetView) return;

            renderer->draw(encoder, targetView, ctx->totalBlades);
            updateGUI(encoder, targetView);
            computeManager->computeMovement(time);

            // Create command buffer
            wgpu::CommandBufferDescriptor cmdBufferDescriptor = {
                .label = "Render operations command buffer"
            };
            // Submit the command buffer
            wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);
            ctx->queue.Submit(1, &command);

            surface.Present();
            frameNumber++;
        }
    }


    void Engine::cleanup()
    {
        ImGui_ImplGlfw_Shutdown();
        ImGui_ImplWGPU_Shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
    }
} // grass
