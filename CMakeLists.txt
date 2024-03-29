cmake_minimum_required(VERSION 3.20)
project("vulkan-adventures")
set (CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)


include_directories("src")
add_executable(vulkan-adventures 
    src/main.cpp 
    src/Camera.cpp 
    src/vulkanohno.cpp
    src/vk_initializers.cpp
    src/PipelineBuilder.cpp 
    src/Mesh.cpp 
    src/RenderObject.cpp
    src/AssetManager.cpp
    src/RasterEngine.cpp
    #src/RaytraceEngine.cpp
    src/vk_funcs.cpp)


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
    fetch_vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    GIT_TAG        v3.0.1
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
FetchContent_MakeAvailable(fetch_vma)
target_link_libraries(vulkan-adventures VulkanMemoryAllocator)

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
    fetch_tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf/
    GIT_TAG        v2.8.14
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
FetchContent_MakeAvailable(fetch_tinygltf)
target_link_libraries(vulkan-adventures tinygltf)


FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(glm)
target_compile_definitions(glm INTERFACE GLM_FORCE_SILENT_WARNINGS)

FetchContent_Declare(
    fetch_imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui/
    GIT_TAG        v1.90.1
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(fetch_imgui)
add_library(imgui)
target_include_directories(imgui PUBLIC ${fetch_imgui_SOURCE_DIR})
target_sources(imgui PRIVATE 
  ${fetch_imgui_SOURCE_DIR}/imgui.h
  ${fetch_imgui_SOURCE_DIR}/imgui.cpp

  ${fetch_imgui_SOURCE_DIR}/imgui_demo.cpp
  ${fetch_imgui_SOURCE_DIR}/imgui_draw.cpp
  ${fetch_imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${fetch_imgui_SOURCE_DIR}/imgui_tables.cpp
  ${fetch_imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
  ${fetch_imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
   )

target_link_libraries(imgui PUBLIC Vulkan::Vulkan SDL2::SDL2)
include_directories(${fetch_imgui_SOURCE_DIR})
include_directories(${fetch_imgui_SOURCE_DIR}/backends)

target_link_libraries(vulkan-adventures imgui)


if (WIN32)
    add_custom_command(
        TARGET vulkan-adventures POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL2::SDL2>" "$<TARGET_FILE_DIR:vulkan-adventures>"
        VERBATIM
    )
endif()


set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")


file(GLOB_RECURSE GLSL_SOURCE_FILES
    "shaders/*.frag"
    "shaders/*.vert"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
    )

add_dependencies(vulkan-adventures Shaders)

add_custom_command(TARGET vulkan-adventures POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:vulkan-adventures>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:vulkan-adventures>/shaders"
        )
