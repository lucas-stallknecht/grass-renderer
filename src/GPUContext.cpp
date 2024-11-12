#include "GPUContext.h"

#include <stdexcept>
#include <iostream>

GPUContext* GPUContext::ctx = nullptr;

GPUContext::GPUContext(GLFWwindow* window)
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
        &device
    );
    instance.WaitAny(deviceFuture, UINT64_MAX);
    assert(device && "Could not request device!");

    wgpu::SurfaceCapabilities capabilities;
    surface.GetCapabilities(chosenAdapter, &capabilities);
    surfaceFormat = capabilities.formats[0];
    assert(surfaceFormat != wgpu::TextureFormat::Undefined && "Wrong surface format!");

    queue = device.GetQueue();
    assert(queue && "Could not get queue from device!");
}


GPUContext* GPUContext::getInstance(GLFWwindow* window)
{
    if (ctx == nullptr)
    {
        if (!window)
        {
            throw std::runtime_error("GPUContext not initialized yet. Please call with a valid GLFWwindow.");
        }
        ctx = new GPUContext(window);
    }
    return ctx;
}


void GPUContext::configSurface(uint32_t width, uint32_t height)
{
    wgpu::SurfaceConfiguration config = {
        .device = device,
        .format = surfaceFormat,
        .usage = wgpu::TextureUsage::RenderAttachment,
        .alphaMode = wgpu::CompositeAlphaMode::Auto,
        .width = width,
        .height = height,
        .presentMode = wgpu::PresentMode::Fifo,
    };
    surface.Configure(&config);
}


wgpu::TextureView GPUContext::getNextSurfaceTextureView()
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
