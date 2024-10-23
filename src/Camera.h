#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace grass
{
    struct CameraUniform
    {
        glm::mat4 viewMatrix;
        glm::mat4 projMatrix;
        glm::vec3 pos;
        float padding1;
        glm::vec3 dir;
        float padding2;
    };

    class Camera
    {
        static constexpr float CAM_MOV_SPEED = 4.3f;
        static constexpr float CAM_VIEW_SPEED = 0.0007f;

    public:
        Camera(float_t fov, float_t aspect, float_t near = 0.1, float_t far = 100.0);
        void updateMatrix();
        void moveForward(float deltaTime);
        void moveBackward(float deltaTime);
        void moveLeft(float deltaTime);
        void moveRight(float deltaTime);
        void moveUp(float deltaTime);
        void moveDown(float deltaTime);
        void updateCamDirection(float xoffset, float yoffset);

        glm::mat4 viewMatrix{};
        glm::mat4 projMatrix{};
        glm::vec3 position{7.4, 1.6, 0.0};
        glm::vec3 direction{-1.0 ,-0.2, 0};
        float_t fov;
        float_t aspect;
        float_t near;
        float_t far;
    };
} // grass
