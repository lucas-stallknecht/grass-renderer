#pragma once

#include <cmath>

namespace grass
{
    constexpr uint16_t WIDTH = 1400;
    constexpr uint16_t HEIGHT = 800;

    struct GrassUniform
    {
        // Field settings
        float_t sideLength = 3;
        float_t density = 10; // blades per unit
        float_t maxNoisePositionOffset = 0.2;
        float_t sizeNoiseFrequency = 0.4;
        // Single blades settings
        float_t bladeHeight = 1.0;
        float_t sizeNoiseAmplitude = 0.3;
    };

    struct GrassSettings
    {
        GrassSettings() { calculateTotal(); }

        void calculateTotal()
        {
            bladesPerSide = static_cast<size_t>(grassUniform.sideLength * grassUniform.density * 2);
            totalBlades = static_cast<size_t>(std::floor(std::pow(bladesPerSide, 2)));
            grassUniform.maxNoisePositionOffset = grassUniform.sideLength * 2.0f / static_cast<float>(bladesPerSide);
        }

        GrassUniform grassUniform{};
        size_t bladesPerSide{};
        size_t totalBlades{};
    };
}
