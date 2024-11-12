#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace grass
{
    struct CameraUniformData
    {
        glm::mat4 viewMatrix;
        glm::mat4 projMatrix;
        glm::vec3 pos;
        float padding1;
        glm::vec3 dir;
        float padding2;
        glm::mat4 invViewMatrix;
        glm::mat4 invProjMatrix;
    };

    class Camera
    {
        static constexpr float CAM_MOV_SPEED = 4.3f;
        static constexpr float CAM_VIEW_SPEED = 0.0007f;

    public:
        Camera(float fov, float aspect, float near = 0.1, float far = 100.0);
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
        glm::vec3 position{3.80, 1.19, 3.14};
        glm::vec3 direction{-0.88, -0.09, -0.51};
        float fov;
        float aspect;
        float near;
        float far;
    };
} // grass
