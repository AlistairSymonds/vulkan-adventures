cmake_minimum_required(VERSION 3.20)
project(vulkan-adventures)


add_executable(vulkan-adventures src/main.cpp)

include(FetchContent)

FetchContent_Declare(
    fetch_vk_bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
    GIT_TAG        v0.7
)
FetchContent_MakeAvailable(fetch_vk_bootstrap)
target_link_libraries(vulkan-adventures vk-bootstrap::vk-bootstrap)

FetchContent_Declare(
    fetch_libsdl
    GIT_REPOSITORY https://github.com/libsdl-org/SDL/
    GIT_TAG        release-2.28.2
)
FetchContent_MakeAvailable(fetch_libsdl)
target_link_libraries(vulkan-adventures SDL2::SDL2)
