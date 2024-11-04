#pragma once

#include <cmath>
#include <glm/glm.hpp>

namespace grass
{
    struct GrassGenUniformData
    {
        // Field settings
        float sideLength = 10.0;
        float density = 20.0; // blades per unit
        float maxNoisePositionOffset = 0.2;
        float sizeNoiseFrequency = 0.35;

        // Single blades settings
        float bladeHeight = 0.9;
        float sizeNoiseAmplitude = 0.4;
    };

    struct GrassMovUniformData
    {
        // vec3 direction + strength
        glm::vec4 wind = {1.0, 0.0, 0.0, 0.4};
        float windFrequency = 0.6;
        glm::vec3 padding;
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

        GrassGenUniformData grassUniform{};
        size_t bladesPerSide{};
        size_t totalBlades{};
    };

    struct BladeStaticUniformData
    {
        glm::vec3 lightCol = {1.0, 1.0, 1.0};
        float wrapValue = 1.0;
        glm::vec3 lightDirection = {-0.7, 1.0, 0.0};
        float diffuseStrength = 0.8;
        glm::vec3 bladeCol = {0.539, 0.156, 0.156};
        float ambientStrength = 0.3;
        glm::vec3 specularCol = {1.0, 0.88, 0.88};
        float specularStrength = 0.15;
    };

    struct BladeDynamicUniformData
    {
        float time = 0.0;
    };
}
