#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace grass
{
    class Camera
    {
    public:
        Camera(float_t fov, float_t aspect, float_t near = 0.1, float_t far = 100.0);
        void updateMatrix();

        glm::mat4 viewProjMatrix{};
        glm::vec3 position{};
        glm::vec3 target{};
        float_t fov;
        float_t aspect;
        float_t near;
        float_t far;
    };
} // grass
