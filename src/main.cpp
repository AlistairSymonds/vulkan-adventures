#include <iostream>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <chrono>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include "Camera.h"
#include "vulkanohno.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

int main() {

    VulkanOhNo vkohno;

    vkohno.init();

    
    vkohno.cam.velocity = glm::vec3(0.f);
    vkohno.cam.position = glm::vec3(0, 0, 10);

    vkohno.cam.pitch = 0;
    vkohno.cam.yaw = 0;

    SDL_Event e;
    bool bQuit = false;
    std::chrono::microseconds loop_time = std::chrono::microseconds();
    glm::vec2 last_mouse_pos = {};

    std::vector<RenderObject> tri_ros;
    for (size_t i = 0; i < 10; i++)
    {
        glm::mat4 transform(1);

        
        RenderObject tri;
        tri.meshId = "tri";
        tri.materialId = "Mesh";
        tri.worldMatrix = glm::translate(transform, glm::vec3(-0.5 + i, -0.5 - i, (0.0f + i / 3)*-1));
        tri_ros.push_back(tri);
    }

    RenderObject grass_field;
    grass_field.meshId = "field";
    grass_field.materialId = "Mesh";
    grass_field.worldMatrix = glm::translate(glm::mat4(1), glm::vec3(0, 0, -3));

    //test the default cube
    RenderObject dummy;
    dummy.meshId = "hmmm I hope this doesn't exist";
    dummy.materialId = "no mat!";
    dummy.worldMatrix = glm::translate(glm::mat4(1), glm::vec3(0, 0, -3));


    //main loop
    while (!bQuit)
    {
        const std::chrono::time_point<std::chrono::steady_clock>  loop_start = std::chrono::steady_clock::now();

        //Handle events on queue
        while (SDL_PollEvent(&e) != 0)
        {
            //close the window when user clicks the X button or alt-f4s
            if (e.type == SDL_QUIT) bQuit = true;

            vkohno.cam.processSDLEvent(e);
            ImGui_ImplSDL2_ProcessEvent(&e);
        }


        std::vector<RenderObject> objs;
        objs.insert(objs.begin(), tri_ros.begin(), tri_ros.end());
        objs.push_back(dummy);
        vkohno.draw(objs);
        const std::chrono::time_point<std::chrono::steady_clock>  loop_end = std::chrono::steady_clock::now();
        loop_time = std::chrono::duration_cast<std::chrono::microseconds> (loop_end - loop_start);
    }
    

    vkohno.cleanup();
    return 0;
}