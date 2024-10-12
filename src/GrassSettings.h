#pragma once

#include <cmath>

namespace grass
{
    constexpr uint16_t WIDTH = 1400;
    constexpr uint16_t HEIGHT = 800;

    struct GrassSettings
    {
        GrassSettings() { calculateTotal(); }
        void calculateTotal()
        {
            bladesPerSide = sideLength * density * 2;
            totalBlades = static_cast<size_t>(std::pow(bladesPerSide, 2));
        }
        size_t sideLength = 2;
        size_t density = 10; // blades per unit
        size_t bladesPerSide{};
        size_t totalBlades{};
    };
}
