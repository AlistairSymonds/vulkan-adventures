#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>

Camera::Camera() {
    pos = { 0.0f, 0.0f, 0.0f };
    rot = { 0.0f, 0.0f, 0.0f };
    NearPlane = 0.01f;
    Fov = 90;
    AspectRatio = 16.f / 9.f;
    FarPlane = 1000000;
}



void Camera::Rotate(float pitch, float yaw)
{
    rot.x += pitch;
    rot.y += yaw;
}


glm::mat4 Camera::GetViewMatrix()
{   
    glm::mat4 viewMatrix = glm::yawPitchRoll(rot.x, rot.y, rot.z);
    glm::vec3 posflip = pos;
    posflip.y *= -1.0;
    viewMatrix = glm::translate(viewMatrix, posflip);
    return viewMatrix;
}

glm::mat4 Camera::GetProjectionMatrix()
{
    auto projectionMatrix = glm::perspective(glm::radians(Fov), AspectRatio, NearPlane, FarPlane);
    projectionMatrix[1][1] *= -1; // Flip Y axis, now it's y-up
    return projectionMatrix;
}

void Camera::MoveTranslate(const glm::vec3)
{
}
