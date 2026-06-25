cmake_minimum_required(VERSION 3.16)
project(3d-renderer VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2 REQUIRED)

add_library(renderer_core
    src/core/asset_locator.cpp
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
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_link_libraries(renderer_core PUBLIC SDL2::SDL2)

target_compile_definitions(renderer_core
    PUBLIC
        RENDERER_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_executable(raytracer src/raytracer/main.cpp)
target_link_libraries(raytracer PRIVATE renderer_core)

add_executable(rasterizer src/rasterizer/main.cpp)
target_link_libraries(rasterizer PRIVATE renderer_core)

add_executable(raycaster src/raycaster/main.cpp)
target_link_libraries(raycaster PRIVATE renderer_core)

add_library(renderer_app_lib
    src/app/app_state.cpp
    src/app/renderer_app.cpp
    src/app/raytracer_rasterizer_controllers.cpp
)
target_link_libraries(renderer_app_lib PUBLIC renderer_core)

add_executable(renderer_app src/app/main.cpp)
target_link_libraries(renderer_app PRIVATE renderer_app_lib)

target_compile_definitions(renderer_app
    PRIVATE
        RENDERER_INSTALL_ASSETS_DIR="${CMAKE_INSTALL_PREFIX}/share/3d-renderer/assets"
)

enable_testing()

add_executable(test_math tests/test_math.cpp)
target_link_libraries(test_math PRIVATE renderer_core)
add_test(NAME math COMMAND test_math)

add_executable(test_raycaster tests/test_raycaster.cpp)
target_link_libraries(test_raycaster PRIVATE renderer_core)
add_test(NAME raycaster_unit COMMAND test_raycaster WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(test_rasterizer tests/test_rasterizer.cpp)
target_link_libraries(test_rasterizer PRIVATE renderer_core)
add_test(NAME rasterizer_unit COMMAND test_rasterizer)

add_executable(test_golden tests/test_golden.cpp)
add_test(
    NAME golden
    COMMAND test_golden
        $<TARGET_FILE:raytracer>
        $<TARGET_FILE:rasterizer>
        $<TARGET_FILE:raycaster>
        ${CMAKE_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_executable(test_asset_locator tests/test_asset_locator.cpp)
target_link_libraries(test_asset_locator PRIVATE renderer_core)
add_test(NAME asset_locator COMMAND test_asset_locator WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(test_app_smoke tests/test_app_smoke.cpp)
target_link_libraries(test_app_smoke PRIVATE renderer_app_lib)
add_test(NAME app_smoke COMMAND test_app_smoke WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

install(TARGETS renderer_app raytracer rasterizer raycaster
    RUNTIME DESTINATION bin
)

install(DIRECTORY assets/
    DESTINATION share/3d-renderer/assets
)

include(GNUInstallDirs)

set(CPACK_PACKAGE_NAME "3d-renderer")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Software 3D renderer desktop application")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_GENERATOR "TGZ;ZIP")

if(WIN32)
    list(APPEND CPACK_GENERATOR "ZIP")
elseif(APPLE)
    list(APPEND CPACK_GENERATOR "TGZ")
else()
    list(APPEND CPACK_GENERATOR "TGZ")
endif()

include(CPack)
