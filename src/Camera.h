#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <SDL_events.h>

//blatantly stolen from:
//https://vkguide.dev/docs/new_chapter_5/interactive_camera/

class Camera {
public:
    glm::vec3 velocity;
    glm::vec3 position;
    // vertical rotation
    float pitch{ 0.f };
    // horizontal rotation
    float yaw{ 0.f };

    glm::mat4 getViewMatrix();
    glm::mat4 getRotationMatrix();

    void processSDLEvent(SDL_Event& e);

    void update();
};
