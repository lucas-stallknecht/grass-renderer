#pragma once

#include <webgpu/webgpu_cpp.h>

class PhongMaterial
{
public:
    explicit PhongMaterial(wgpu::Texture diff);

    wgpu::Texture diffuseTexture;
    wgpu::BindGroup bindGroup;

private:
    void createBindGroup();
};
