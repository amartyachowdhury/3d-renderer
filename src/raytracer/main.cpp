#include "renderer/core/framebuffer.h"
#include "renderer/platform/sdl_window.h"
#include "renderer/raytracer/material.h"
#include "renderer/raytracer/scene_loader.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

struct Options {
    std::string output = "output.ppm";
    std::string scene_path;
    bool preview = false;
    bool dump_only = false;
    int width = 800;
    int height = 450;
    int samples = 10;
    int max_depth = 50;
    double camera_yaw = 0.0;
};

Options parse_args(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--output" && i + 1 < argc) {
            options.output = argv[++i];
        } else if (arg == "--scene" && i + 1 < argc) {
            options.scene_path = argv[++i];
        } else if (arg == "--preview") {
            options.preview = true;
        } else if (arg == "--dump-ppm") {
            options.dump_only = true;
        } else if (arg == "--width" && i + 1 < argc) {
            options.width = std::stoi(argv[++i]);
        } else if (arg == "--height" && i + 1 < argc) {
            options.height = std::stoi(argv[++i]);
        } else if (arg == "--samples" && i + 1 < argc) {
            options.samples = std::stoi(argv[++i]);
        } else if (arg == "--max-depth" && i + 1 < argc) {
            options.max_depth = std::stoi(argv[++i]);
        } else if (arg == "--camera-yaw" && i + 1 < argc) {
            options.camera_yaw = std::stod(argv[++i]);
        }
    }
    return options;
}

}  // namespace

int main(int argc, char** argv) {
    using namespace renderer;

    const Options options = parse_args(argc, argv);

    SceneDescription scene_desc;
    const int cli_width = options.width;
    const int cli_height = options.height;
    const int cli_samples = options.samples;
    const int cli_max_depth = options.max_depth;

    std::vector<std::unique_ptr<Material>> materials;
    HittableList world;

    if (!options.scene_path.empty()) {
        std::string error;
        if (!load_scene(options.scene_path, scene_desc, error)) {
            std::cerr << error << '\n';
            return 1;
        }
        if (cli_width != 800) {
            scene_desc.width = cli_width;
        }
        if (cli_height != 450) {
            scene_desc.height = cli_height;
        }
        if (cli_samples != 10) {
            scene_desc.samples = cli_samples;
        }
        if (cli_max_depth != 50) {
            scene_desc.max_depth = cli_max_depth;
        }
        build_scene(scene_desc, world, materials);
    } else {
        scene_desc.width = cli_width;
        scene_desc.height = cli_height;
        scene_desc.samples = cli_samples;
        scene_desc.max_depth = cli_max_depth;
        world = build_default_scene(materials);
    }

    const double aspect = static_cast<double>(scene_desc.width) / static_cast<double>(scene_desc.height);
    const double yaw = options.camera_yaw * M_PI / 180.0;
    const double radius = (scene_desc.look_from - scene_desc.look_at).length();
    Point3 look_from{
        scene_desc.look_at.x + radius * std::sin(yaw),
        scene_desc.look_from.y,
        scene_desc.look_at.z + radius * std::cos(yaw),
    };

    Camera camera(
        look_from,
        scene_desc.look_at,
        scene_desc.vup,
        scene_desc.vfov,
        aspect,
        scene_desc.aperture,
        scene_desc.focus_dist);

    Framebuffer framebuffer(scene_desc.width, scene_desc.height);
    std::unique_ptr<SdlWindow> window;
    if (options.preview && !options.dump_only) {
        window = std::make_unique<SdlWindow>("Ray Tracer", scene_desc.width, scene_desc.height);
    }

    const auto start = std::chrono::steady_clock::now();

    for (int y = 0; y < scene_desc.height; ++y) {
        if (window) {
            window->poll_events();
            if (window->should_close()) {
                break;
            }
        }

        std::cerr << "\rScanlines remaining: " << (scene_desc.height - y - 1) << ' ' << std::flush;

        for (int x = 0; x < scene_desc.width; ++x) {
            Color pixel{0.0, 0.0, 0.0};
            for (int sample = 0; sample < scene_desc.samples; ++sample) {
                const double u = (x + random_double()) / (scene_desc.width - 1);
                const double v = (y + random_double()) / (scene_desc.height - 1);
                Ray ray = camera.get_ray(u, v);
                pixel += ray_color(ray, world, scene_desc.max_depth);
            }
            pixel /= static_cast<double>(scene_desc.samples);
            framebuffer.set_pixel(x, scene_desc.height - 1 - y, pixel);
        }

        if (window && y % 4 == 0) {
            window->blit(framebuffer);
        }
    }

    std::cerr << "\nDone.\n";

    if (!framebuffer.write_ppm(options.output)) {
        std::cerr << "Failed to write " << options.output << '\n';
        return 1;
    }

    if (window) {
        window->blit(framebuffer);
        while (window->is_open() && !window->should_close()) {
            window->poll_events();
        }
    }

    const auto end = std::chrono::steady_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Wrote " << options.output << " in " << ms << " ms\n";
    return 0;
}
