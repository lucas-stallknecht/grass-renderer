#pragma once

#include <cmath>
#include <glm/glm.hpp>

namespace grass
{
    struct GrassGenUniformData
    {
        // Field settings
        float sideLength = 3.0;
        float density = 12; // blades per unit
        float maxNoisePositionOffset = 0.2;
        float sizeNoiseFrequency = 0.75;

        // Single blades settings
        float bladeHeight = 0.9;
        float sizeNoiseAmplitude = 0.4;
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
        // vec3 direction + strength
        glm::vec4 wind = {1.0, 0.0, 0.0, 0.4};

        glm::vec3 lightDirection = {-0.7, 1.0, 0.3};
        float windFrequency = 0.6;

        glm::vec3 lightCol = {1.0, 1.0, 1.0};
        float wrapValue = 1.0;

        glm::vec3 bladeCol = {0.65, 0.21, 0.17};
        float ambientStrength = 0.3;

        glm::vec3 specularCol = {0.88, 0.88, 1.0};
        float diffuseStrength = 0.8;

        float specularStrength = 0.3;
        glm::vec3 padding;
    };

    struct BladeDynamicUniformData
    {
        float time = 0.0;
    };
}
