#pragma once

#include <cmath>
#include <glm/glm.hpp>
#include "Camera.h"

namespace grass
{
    // Compute uniforms
    struct GrassGenUniformData
    {
        // Field settings
        float sideLength = 10.0;
        float density = 15.0; // blades per unit
        float maxNoisePositionOffset = 0.2;
        float sizeNoiseFrequency = 0.3;

        // Single blades settings
        float bladeHeight = 0.9;
        float sizeNoiseAmplitude = 0.4;
    };

    struct GrassMovUniformData
    {
        // vec3 direction + strength
        glm::vec4 wind = {0.8, 0.0, -0.5, 0.75};
        float windFrequency = 0.8;
        glm::vec3 padding;
    };

    // Rendering uniforms
    struct LightUniformData
    {
        glm::vec3 skyUpCol = {0.275, 0.659, 1.00};
        float padding1;
        glm::vec3 skyGroundCol = {0.863, 0.952, 1.0};
        float padding2;
        glm::vec3 sunCol = {1.0, 0.886, 0.716};
        float padding3;
        glm::vec3 sunDir = {0.2, 1.0, 0.2};
        float padding4;
    };

    struct GlobalUniformData
    {
        CameraUniformData camera;
        LightUniformData light;
        float time;
        uint32_t frameNumber;
        glm::vec2 padding;
    };


    struct BladeStaticUniformData
    {
        glm::vec3 smallerBladeCol = {0.794, 0.641, 0.311};
        float ambientStrength = 0.3;
        glm::vec3 tallerBladeCol = {1.0, 0.913, 0.725};
        float wrapValue = 1.0;
        glm::vec3 specularCol = {1.0, 0.968, 0.863};
        float specularStrength = 0.15;
        float diffuseStrength = 0.8;
        float shadows = 1.0; // 0.0 or 1.0
        glm::vec2 padding;
    };

    struct ScreenSpaceShadowsUniformData
    {
        uint32_t max_steps = 32;
        float ray_max_distance = 2.00;
        float thickness = 0.1;
        float max_delta_from_original_depth = 0.01;
    };
}
