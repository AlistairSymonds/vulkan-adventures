cmake_minimum_required(VERSION 3.20)
project("vulkan-adventures")
set (CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)


include_directories("src")
add_executable(vulkan-adventures src/main.cpp "src/vulkanohno.cpp")


target_link_libraries(vulkan-adventures Vulkan::Vulkan)

include(FetchContent)

FetchContent_Declare(
    fetch_vk_bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
    GIT_TAG        v0.7
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
FetchContent_MakeAvailable(fetch_vk_bootstrap)
target_link_libraries(vulkan-adventures vk-bootstrap::vk-bootstrap)

FetchContent_Declare(
    fetch_libsdl
    GIT_REPOSITORY https://github.com/libsdl-org/SDL/
    GIT_TAG        release-2.28.2
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
FetchContent_MakeAvailable(fetch_libsdl)
target_link_libraries(vulkan-adventures SDL2::SDL2)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

message("Fetching glm")
FetchContent_MakeAvailable(glm)
target_compile_definitions(glm INTERFACE GLM_FORCE_SILENT_WARNINGS)

if (WIN32)
    add_custom_command(
        TARGET vulkan-adventures POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL2::SDL2>" "$<TARGET_FILE_DIR:vulkan-adventures>"
        VERBATIM
    )
endif()