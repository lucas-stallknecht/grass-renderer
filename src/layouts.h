#pragma once

#include <webgpu/webgpu_cpp.h>
#include "GlobalConfig.h"

// --------- VERTEX ATTRIBUTES ----------
inline constexpr wgpu::VertexAttribute defaultVertexAttribs[3] = {
    {
        .format = wgpu::VertexFormat::Float32x3,
        .offset = 0,
        .shaderLocation = 0,
    },
    {
        .format = wgpu::VertexFormat::Float32x3,
        .offset = 3 * sizeof(float),
        .shaderLocation = 1,
    },
    {
        .format = wgpu::VertexFormat::Float32x2,
        .offset = 6 * sizeof(float),
        .shaderLocation = 2,
    }
};

inline constexpr wgpu::VertexBufferLayout defaultVertexLayout = {
    .arrayStride = 8 * sizeof(float),
    .stepMode = wgpu::VertexStepMode::Vertex,
    .attributeCount = 3,
    .attributes = &defaultVertexAttribs[0]
};


// --------- DEPTH STENCIL ----------
inline constexpr wgpu::DepthStencilState defaultDepthStencil = {
    .format = wgpu::TextureFormat::Depth24Plus,
    .depthWriteEnabled = true,
    .depthCompare = wgpu::CompareFunction::Less,
    .stencilReadMask = 0,
    .stencilWriteMask = 0
};

// --------- BIND GROUP LAYOUTS ----------
inline constexpr wgpu::BindGroupLayoutEntry globalBindGroupLayoutEntry[2] = {
    {
        .binding = 0,
        .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
        .buffer = {
            .type = wgpu::BufferBindingType::Uniform,
            .minBindingSize = sizeof(grass::GlobalUniformData)
        }
    },
    {
        .binding = 1,
        .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
        .sampler = {
            .type = wgpu::SamplerBindingType::Filtering,
        }
    }
};
inline constexpr wgpu::BindGroupLayoutDescriptor globalBindGroupLayoutDesc = {
    .label = "Global uniforms bind group layout",
    .entryCount = 2,
    .entries = &globalBindGroupLayoutEntry[0]
};


inline constexpr wgpu::BindGroupLayoutEntry phongMaterialBindGroupLayoutEntry[2] = {
    {
        .binding = 0,
        .visibility = wgpu::ShaderStage::Fragment,
        .texture = {
            .sampleType = wgpu::TextureSampleType::Float,
            .viewDimension = wgpu::TextureViewDimension::e2D
        },
    }
};
inline constexpr wgpu::BindGroupLayoutDescriptor phongMaterialBindGroupLayoutDesc = {
    .label = "Material uniforms bind group layout",
    .entryCount = 1,
    .entries = &phongMaterialBindGroupLayoutEntry[0]
};


inline constexpr wgpu::BindGroupLayoutEntry modelBindGroupLayoutEntry[2] = {
    {
        .binding = 0,
        .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
        .buffer = {
            .type = wgpu::BufferBindingType::Uniform,
            .minBindingSize = sizeof(glm::mat4)
        }
    },
};
inline constexpr wgpu::BindGroupLayoutDescriptor modelBindGroupLayoutDesc = {
    .label = "Model uniforms bind group layout",
    .entryCount = 1,
    .entries = &modelBindGroupLayoutEntry[0]
};
