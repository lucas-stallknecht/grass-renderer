#pragma once

#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu_cpp.h>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu_glfw.h>
#include <dawn/native/DawnNative.h>

#include "GrassSettings.h"
#include "Camera.h"
#include "Renderer.h"
#include "ComputeManager.h"


namespace grass
{
    class Engine
    {

    public:
        static Engine& Get();

        void init();
        void run();
        void cleanup();

    private:
        void initWindow();
        void initWebgpu();
        void configSurface();
        wgpu::TextureView getNextSurfaceTextureView();

        std::unique_ptr<Renderer> renderer;
        std::unique_ptr<ComputeManager> computeManager;

        // Global
        wgpu::Instance instance;
        wgpu::Device device;
        wgpu::Queue queue;
        wgpu::Surface surface;
        wgpu::TextureFormat surfaceFormat = wgpu::TextureFormat::Undefined;

        GLFWwindow* window = nullptr;
        Camera camera{35.0, WIDTH / static_cast<float_t>(HEIGHT)};
        uint32_t frameNumber = 0;
        GrassSettings grassSettings{};
    };
} // grass
