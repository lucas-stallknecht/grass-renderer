#pragma once
#include "uniforms.h"

namespace grass {

    struct Context {
        Context() { calculateTotal(); }
        void calculateTotal()
        {
            bladesPerSide = static_cast<size_t>(grassUniform.sideLength * grassUniform.density * 2);
            totalBlades = static_cast<size_t>(std::floor(std::pow(bladesPerSide, 2)));
            grassUniform.maxNoisePositionOffset = grassUniform.sideLength * 2.0f / static_cast<float>(bladesPerSide);
        }

        wgpu::Device device;
        wgpu::Queue queue;
        wgpu::TextureFormat surfaceFormat = wgpu::TextureFormat::Undefined;

        GrassGenUniformData grassUniform{};
        GrassMovUniformData movUniform{};
        BladeStaticUniformData bladeUniform{};
        size_t bladesPerSide{};
        size_t totalBlades{};
    };
}