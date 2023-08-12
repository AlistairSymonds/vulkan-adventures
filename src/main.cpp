#include <iostream>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <glm/gtx/string_cast.hpp>
#include "Camera.h"
#include "vulkanohno.h"


int main() {

    VulkanOhNo vkohno;

    vkohno.init();

    
    SDL_Event e;
    bool bQuit = false;

    //main loop
    while (!bQuit)
    {
        //Handle events on queue
        while (SDL_PollEvent(&e) != 0)
        {
            //close the window when user clicks the X button or alt-f4s
            if (e.type == SDL_QUIT) bQuit = true;
            
            float move_speed = 1.0f;
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                case SDLK_w:
                    vkohno.cam.pos.x += 1 * move_speed;
                    break;
                case SDLK_a:
                    vkohno.cam.pos.y += -1 * move_speed;
                    break;
                case SDLK_s:
                    vkohno.cam.pos.x += -1 * move_speed;
                    break;
                case SDLK_d:
                    vkohno.cam.pos.y += 1 * move_speed;
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

        vkohno.draw();
    }
    

    vkohno.cleanup();
    return 0;
}