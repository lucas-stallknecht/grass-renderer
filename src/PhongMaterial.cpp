#include "PhongMaterial.h"

#include "GPUContext.h"
#include "layouts.h"

PhongMaterial::PhongMaterial(wgpu::Texture diff) : diffuseTexture(std::move(diff))
{
    createBindGroup();
}

void PhongMaterial::createBindGroup()
{
    wgpu::TextureViewDescriptor textureViewDesc = {
        .format = diffuseTexture.GetFormat(),
        .dimension = wgpu::TextureViewDimension::e2D,
        .mipLevelCount = 1,
        .arrayLayerCount = 1
    };
    wgpu::TextureView textureView = diffuseTexture.CreateView(&textureViewDesc);


    wgpu::BindGroupEntry entry[1] = {
        {
            .binding = 0,
            .textureView = textureView,
        },
    };
    wgpu::BindGroupDescriptor bindGroupDesc = {
        .label = "Phong material bind group",
        .layout = GPUContext::getInstance()->getDevice().CreateBindGroupLayout(&phongMaterialBindGroupLayoutDesc),
        .entryCount = 1,
        .entries = &entry[0]
    };
    bindGroup = GPUContext::getInstance()->getDevice().CreateBindGroup(&bindGroupDesc);
}
