#pragma once

#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu_cpp.h>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu_glfw.h>
#include <dawn/native/DawnNative.h>



namespace grass
{
    constexpr unsigned int WIDTH = 800;
    constexpr unsigned int HEIGHT = 600;

    class Engine
    {

    public:
        static Engine& Get();

        void init();
        void run();
        void cleanup();

    private:
        void initWebgpu();
        void initWindow();
        void configSurface();
        void createTestBuffer();
        void createPipeline();
        wgpu::TextureView getNextSurfaceTextureView();
        void draw();

        wgpu::Device device;
        wgpu::Queue queue;
        wgpu::Surface surface;
        wgpu::TextureFormat surfaceFormat;
        wgpu::Buffer testBuffer;
        wgpu::RenderPipeline simplePipeline;

        GLFWwindow* window;
        uint32_t frameNmber = 0;


    };
} // grass
