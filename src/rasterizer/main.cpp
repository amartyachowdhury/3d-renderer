#include "renderer/core/framebuffer.h"
#include "renderer/core/texture.h"
#include "renderer/math/mat4.h"
#include "renderer/platform/sdl_window.h"
#include "renderer/rasterizer/mesh.h"
#include "renderer/rasterizer/rasterizer.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(int argc, char** argv) {
    using namespace renderer;

    std::string obj_path;
    std::string texture_path;
    std::string output = "rasterizer.ppm";
    bool dump_only = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--obj" && i + 1 < argc) {
            obj_path = argv[++i];
        } else if (arg == "--texture" && i + 1 < argc) {
            texture_path = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            output = argv[++i];
        } else if (arg == "--dump-ppm") {
            dump_only = true;
        }
    }

    const int width = 960;
    const int height = 540;

    Mesh mesh;
    std::string error;
    if (!obj_path.empty()) {
        if (!load_obj(obj_path, mesh, error)) {
            std::cerr << error << '\n';
            return 1;
        }
    } else {
        mesh = make_cube();
    }

    Texture texture;
    const Texture* texture_ptr = nullptr;
    if (!texture_path.empty()) {
        if (!load_ppm_texture(texture_path, texture, error)) {
            std::cerr << error << '\n';
            return 1;
        }
        texture_ptr = &texture;
    }

    SoftwareRasterizer rasterizer(width, height);
    Framebuffer framebuffer(width, height);
    SdlWindow window("Software Rasterizer", width, height);

    const Point3 target{0.0, 0.0, 0.0};
    const Vec3 up{0.0, 1.0, 0.0};
    const Mat4 projection = Mat4::perspective(60.0 * M_PI / 180.0, static_cast<double>(width) / height, 0.1, 100.0);

    const auto start = std::chrono::steady_clock::now();
    double angle = 0.0;

    while (window.is_open() && !window.should_close()) {
        window.poll_events();

        angle += 0.015;
        const Point3 eye{std::sin(angle) * 3.0, 1.5, std::cos(angle) * 3.0};
        const Mat4 view = Mat4::look_at(eye, target, up);
        const Mat4 model = Mat4::rotation_y(angle * 0.5);

        rasterizer.clear({0.05, 0.07, 0.12});
        rasterizer.draw_mesh(mesh, model, view, projection, Light{}, texture_ptr);
        rasterizer.present(framebuffer);
        window.blit(framebuffer);

        if (dump_only) {
            break;
        }
    }

    framebuffer.write_ppm(output);
    const auto end = std::chrono::steady_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Wrote " << output << " (" << ms << " ms interactive loop)\n";
    return 0;
}
