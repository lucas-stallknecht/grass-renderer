#pragma once

#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu_cpp.h>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu_glfw.h>
#include <dawn/native/DawnNative.h>
#include <cmath>

#include "Camera.h"


namespace grass
{

    struct GrassSettings
    {
        GrassSettings() { calculateTotal(); }
        void calculateTotal()
        {
            bladesPerSide = sideLength * density * 2;
            totalBlades = static_cast<size_t>(std::pow(bladesPerSide, 2));
        }
        size_t sideLength = 2;
        size_t density = 5; // blades per unit
        size_t bladesPerSide{};
        size_t totalBlades{};
    };

    constexpr uint16_t WIDTH = 1400;
    constexpr uint16_t HEIGHT = 800;

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
        void createVertexBuffer();
        void createUniformBuffer();
        void createComputeBuffer();
        void initRenderPipeline();
        void createDepthTextureView();
        void initComputPipeline();
        wgpu::TextureView getNextSurfaceTextureView();
        void computeGrassBladePositions();
        void draw();

        // Global
        wgpu::Instance instance;
        wgpu::Device device;
        wgpu::Queue queue;
        wgpu::Surface surface;
        wgpu::TextureFormat surfaceFormat;

        // Render
        wgpu::Buffer vertexBuffer;
        size_t vertexCount = 0;
        wgpu::Buffer uniformBuffer;
        wgpu::RenderPipeline grassPipeline;
        wgpu::BindGroup grassBindGroup;
        wgpu::TextureView depthView;

        // Compute
        wgpu::ComputePipeline computePipeline;
        wgpu::Buffer bladesPositionBufferCompute;
        wgpu::Buffer grassSettingsUniformBuffer;
        wgpu::BindGroup computeBindGroup;

        GLFWwindow* window = nullptr;
        Camera camera{35.0, WIDTH / static_cast<float_t>(HEIGHT)};
        uint32_t frameNumber = 0;
        GrassSettings grassSettings{};
    };
} // grass
