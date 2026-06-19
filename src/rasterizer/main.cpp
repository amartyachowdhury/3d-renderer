#include "renderer/core/cli.h"
#include "renderer/core/framebuffer.h"
#include "renderer/core/texture.h"
#include "renderer/math/constants.h"
#include "renderer/math/mat4.h"
#include "renderer/platform/sdl_window.h"
#include "renderer/rasterizer/mesh.h"
#include "renderer/rasterizer/rasterizer.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>

namespace {

void print_help(const char* program) {
    renderer::print_usage(
        program,
        "[options]\n"
        "  --obj PATH         Load OBJ mesh (default: cube)\n"
        "  --texture PATH     Diffuse texture (PNG/PPM)\n"
        "  --output PATH      Output image path (.png or .ppm)\n"
        "  --wireframe        Overlay triangle wireframe\n"
        "  --dump-ppm         Render one frame and exit\n"
        "  --help             Show this help");
}

struct Options {
    std::string obj_path;
    std::string texture_path;
    std::string output = "rasterizer.ppm";
    bool dump_only = false;
    bool wireframe = false;
    bool show_help = false;
};

Options parse_args(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            options.show_help = true;
        } else if (arg == "--obj" && i + 1 < argc) {
            options.obj_path = argv[++i];
        } else if (arg == "--texture" && i + 1 < argc) {
            options.texture_path = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            options.output = argv[++i];
        } else if (arg == "--wireframe") {
            options.wireframe = true;
        } else if (arg == "--dump-ppm") {
            options.dump_only = true;
        }
    }
    return options;
}

}  // namespace

int main(int argc, char** argv) {
    using namespace renderer;

    const Options options = parse_args(argc, argv);
    if (options.show_help) {
        print_help(argv[0]);
        return 0;
    }

    const int width = 960;
    const int height = 540;

    Mesh mesh;
    std::string error;
    if (!options.obj_path.empty()) {
        if (!load_obj(options.obj_path, mesh, error)) {
            std::cerr << error << '\n';
            return 1;
        }
    } else {
        mesh = make_cube();
    }

    Texture texture;
    const Texture* texture_ptr = nullptr;
    if (!options.texture_path.empty()) {
        if (!load_texture(options.texture_path, texture, error)) {
            std::cerr << error << '\n';
            return 1;
        }
        texture_ptr = &texture;
    }

    SoftwareRasterizer rasterizer(width, height);
    Framebuffer framebuffer(width, height);

    const Point3 target{0.0, 0.0, 0.0};
    const Vec3 up{0.0, 1.0, 0.0};
    const Mat4 projection = Mat4::perspective(60.0 * kPi / 180.0, static_cast<double>(width) / height, 0.1, 100.0);

    const auto start = std::chrono::steady_clock::now();
    double orbit_yaw = 0.8;
    double orbit_pitch = 0.35;
    const double orbit_distance = 4.0;
    double auto_spin = 0.0;

    const auto render_frame = [&]() {
        const double yaw = orbit_yaw + auto_spin;
        const Point3 eye{
            target.x + orbit_distance * std::cos(orbit_pitch) * std::sin(yaw),
            target.y + orbit_distance * std::sin(orbit_pitch),
            target.z + orbit_distance * std::cos(orbit_pitch) * std::cos(yaw),
        };

        const Mat4 view = Mat4::look_at(eye, target, up);
        const Mat4 model = Mat4::rotation_y(yaw * 0.5);

        rasterizer.clear({0.05, 0.07, 0.12});
        rasterizer.draw_mesh(mesh, model, view, projection, Light{}, texture_ptr);
        if (options.wireframe) {
            rasterizer.draw_wireframe_mesh(mesh, model, view, projection);
        }
        rasterizer.present(framebuffer);
    };

    if (options.dump_only) {
        render_frame();
        if (!framebuffer.write_image(options.output)) {
            std::cerr << "Failed to write " << options.output << '\n';
            return 1;
        }

        const auto end = std::chrono::steady_clock::now();
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "Wrote " << options.output << " (" << ms << " ms)\n";
        return 0;
    }

    SdlWindow window("Software Rasterizer", width, height);

    while (window.is_open() && !window.should_close()) {
        window.poll_events();

        if (window.mouse_left_down()) {
            orbit_yaw += window.mouse_delta_x() * 0.008;
            orbit_pitch += window.mouse_delta_y() * 0.008;
            orbit_pitch = std::clamp(orbit_pitch, -1.4, 1.4);
        } else {
            auto_spin += 0.015;
        }
        window.clear_mouse_delta();

        render_frame();
        window.blit(framebuffer);
    }

    if (!framebuffer.write_image(options.output)) {
        std::cerr << "Failed to write " << options.output << '\n';
        return 1;
    }

    const auto end = std::chrono::steady_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Wrote " << options.output << " (" << ms << " ms interactive loop)\n";
    return 0;
}
