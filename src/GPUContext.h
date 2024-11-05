#pragma once
#include <webgpu/webgpu_cpp.h>

#include "webgpu/webgpu_glfw.h"

class GPUContext {

public:
    // should not be cloneable nor assignable
    GPUContext(GPUContext &other) = delete;
    void operator=(const GPUContext &) = delete;

    static GPUContext *getInstance(GLFWwindow* window = nullptr);

    void configSurface(uint32_t width, uint32_t height);
    wgpu::TextureView getNextSurfaceTextureView();

    wgpu::Device getDevice() { return device; }
    wgpu::Queue getQueue() { return queue; }
    wgpu::Surface getSurface() { return surface; }
    wgpu::TextureFormat getSurfaceFormat() { return surfaceFormat; }



protected:
    explicit GPUContext(GLFWwindow* window);
    static GPUContext* ctx;
    wgpu::Instance instance;
    wgpu::Device device;
    wgpu::Queue queue;
    wgpu::Surface surface;
    wgpu::TextureFormat surfaceFormat = wgpu::TextureFormat::Undefined;
};
