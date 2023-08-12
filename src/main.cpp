#include <iostream>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <glm/gtx/string_cast.hpp>
#include <chrono>
#include "Camera.h"
#include "vulkanohno.h"


int main() {

    VulkanOhNo vkohno;

    vkohno.init();

    vkohno.cam.pos.z = -3;
    SDL_Event e;
    bool bQuit = false;
    std::chrono::microseconds loop_time = std::chrono::microseconds();
    glm::vec2 last_mouse_pos = {};

    //main loop
    while (!bQuit)
    {
        const std::chrono::time_point<std::chrono::steady_clock>  loop_start = std::chrono::steady_clock::now();

        //Handle events on queue
        while (SDL_PollEvent(&e) != 0)
        {
            //close the window when user clicks the X button or alt-f4s
            if (e.type == SDL_QUIT) bQuit = true;
            
            float move_speed = 0.1f;
            

            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {

                
                case SDLK_w:
                    vkohno.cam.pos.z += 1 * move_speed;
                    break;
                case SDLK_a:
                    vkohno.cam.pos.x += 1 * move_speed;
                    break;
                case SDLK_s:
                    vkohno.cam.pos.z += -1 * move_speed;
                    break;
                case SDLK_d:
                    vkohno.cam.pos.x += -1 * move_speed;
                    break;
                case SDLK_SPACE:
                    vkohno.cam.pos.y += 1 * move_speed;
                    break;
                case SDLK_LCTRL:
                    vkohno.cam.pos.y += -1 * move_speed;
                    break;

                case SDLK_UP:
                    vkohno.IncrementPipeline();
                    break;
                default:
                    break;
                }

                std::cout << "Cam pos: " << glm::to_string(vkohno.cam.pos) << std::endl;
            }
            
            
        }

       
        float mouse_sens = 0.001f;
        int mx, my;
        auto mbstate = SDL_GetRelativeMouseState(&mx, &my);

        glm::vec2 cur_mouse_pos;
        cur_mouse_pos.x = mx;
        cur_mouse_pos.y = my;
        if (mbstate & SDL_BUTTON_LMASK) {


            glm::vec2 del = last_mouse_pos - cur_mouse_pos;

            vkohno.cam.Rotate(mx * -mouse_sens, my * -mouse_sens);
        }
        last_mouse_pos = cur_mouse_pos;


        vkohno.draw();
        const std::chrono::time_point<std::chrono::steady_clock>  loop_end = std::chrono::steady_clock::now();
        loop_time = std::chrono::duration_cast<std::chrono::microseconds> (loop_end - loop_start);
    }
    

    vkohno.cleanup();
    return 0;
}