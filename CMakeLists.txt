cmake_minimum_required(VERSION 3.16)
project(3d-renderer VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2 REQUIRED)

add_library(renderer_core
    src/core/framebuffer.cpp
    src/core/texture.cpp
    src/platform/sdl_window.cpp
    src/raytracer/scene_loader.cpp
    src/raytracer/mesh_bvh.cpp
    src/rasterizer/mesh.cpp
    src/rasterizer/rasterizer.cpp
    src/raycaster/map.cpp
    src/raycaster/raycaster_engine.cpp
)

target_include_directories(renderer_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(renderer_core PUBLIC SDL2::SDL2)

add_executable(raytracer src/raytracer/main.cpp)
target_link_libraries(raytracer PRIVATE renderer_core)

add_executable(rasterizer src/rasterizer/main.cpp)
target_link_libraries(rasterizer PRIVATE renderer_core)

add_executable(raycaster src/raycaster/main.cpp)
target_link_libraries(raycaster PRIVATE renderer_core)

enable_testing()

add_executable(test_math tests/test_math.cpp)
target_link_libraries(test_math PRIVATE renderer_core)
add_test(NAME math COMMAND test_math)
