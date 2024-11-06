#pragma once

#include <cmath>
#include <glm/glm.hpp>
#include "Camera.h"

namespace grass
{
    struct GlobalUniformData
    {
        CameraUniformData camera;
        float time;
        glm::vec3 padding;
    };

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
}
