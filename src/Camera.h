#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


class Camera
{
public:
	Camera();

	glm::mat4 GetViewMatrix();
	glm::mat4 GetProjectionMatrix();


    void MoveTranslate(const glm::vec3);
    void Rotate(float pitch, float yaw);


    glm::vec3 pos;
    glm::vec3 rot;
    float Fov;               // Field of view (in degrees)
    float AspectRatio;      // Aspect ratio of the viewport
    float NearPlane;        // Near clipping plane
    float FarPlane;         // Far clipping plane
private:
};