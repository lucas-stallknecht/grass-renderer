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
    constexpr uint16_t WIDTH = 800;
    constexpr uint16_t HEIGHT = 600;
    constexpr float_t AXIS_BOUND = 5.0;
    constexpr size_t density = 5;

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
        size_t numberOfBlades = std::pow(2 * AXIS_BOUND * density, 2);

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
        wgpu::BindGroup computeBindGroup;

        GLFWwindow* window = nullptr;
        Camera camera{35.0, WIDTH / static_cast<float_t>(HEIGHT)};
        uint32_t frameNmber = 0;


    };
} // grass
