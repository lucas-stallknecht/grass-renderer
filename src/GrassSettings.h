#pragma once

#include <cmath>
#include <glm/glm.hpp>

namespace grass
{
    constexpr uint16_t WIDTH = 1400;
    constexpr uint16_t HEIGHT = 800;

    struct GrassComputeUniforms
    {
        // Field settings
        float_t sideLength = 10.0;
        float_t density = 10; // blades per unit
        float_t maxNoisePositionOffset = 0.2;
        float_t sizeNoiseFrequency = 0.75;
        // Single blades settings
        float_t bladeHeight = 0.9;
        float_t sizeNoiseAmplitude = 0.4;
    };

    struct GrassGenerationSettings
    {
        GrassGenerationSettings() { calculateTotal(); }

        void calculateTotal()
        {
            bladesPerSide = static_cast<size_t>(grassUniform.sideLength * grassUniform.density * 2);
            totalBlades = static_cast<size_t>(std::floor(std::pow(bladesPerSide, 2)));
            grassUniform.maxNoisePositionOffset = grassUniform.sideLength * 2.0f / static_cast<float>(bladesPerSide);
        }

        GrassComputeUniforms grassUniform{};
        size_t bladesPerSide{};
        size_t totalBlades{};
    };

    struct GrassVertexSettingsUniforms
    {
        glm::vec3 windDirection = {1.0, 0.0, 0.0};
        float_t p1;
        glm::vec3 lightDirection = {-0.7, 1.0, 0.3};
        float_t p2;
        float_t windFrequency = 0.6;
        float_t windStrength = 0.5;
        float_t time = 0.0;
        float_t p3;
    };
}
