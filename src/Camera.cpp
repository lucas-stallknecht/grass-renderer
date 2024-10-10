#include "Camera.h"

#include <numbers>


namespace grass
{
    Camera::Camera(float_t fov, float_t aspect, float_t near, float_t far) : fov(fov), aspect(aspect), near(near),
                                                                              far(far)
    {
        updateMatrix();
    }

    void Camera::updateMatrix()
    {
        glm::mat4 proj = glm::perspective(glm::radians(fov), aspect, near, far);
        glm::mat4 view = glm::lookAt(position, target, glm::vec3{0.0, 1.0, 0.0});
        viewProjMatrix = proj * view;
    }

} // grass
