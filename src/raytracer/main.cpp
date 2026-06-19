#include "renderer/core/framebuffer.h"
#include "renderer/platform/sdl_window.h"
#include "renderer/raytracer/bvh.h"
#include "renderer/raytracer/material.h"
#include "renderer/raytracer/mesh_bvh.h"
#include "renderer/raytracer/scene_loader.h"
#include "renderer/rasterizer/mesh.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

struct Options {
    std::string output = "output.ppm";
    std::string scene_path;
    std::string obj_path;
    bool preview = false;
    bool dump_only = false;
    int width = 800;
    int height = 450;
    int samples = 10;
    int max_depth = 50;
    int threads = 0;
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
        } else if (arg == "--obj" && i + 1 < argc) {
            options.obj_path = argv[++i];
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
        } else if (arg == "--threads" && i + 1 < argc) {
            options.threads = std::stoi(argv[++i]);
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
    std::unique_ptr<Hittable> world;

    if (!options.scene_path.empty()) {
        std::string error;
        if (!load_scene(options.scene_path, scene_desc, error)) {
            std::cerr << error << '\n';
            return 1;
        }
        if (cli_width != 800) scene_desc.width = cli_width;
        if (cli_height != 450) scene_desc.height = cli_height;
        if (cli_samples != 10) scene_desc.samples = cli_samples;
        if (cli_max_depth != 50) scene_desc.max_depth = cli_max_depth;

        HittableList list;
        build_scene(scene_desc, list, materials);
        world = std::make_unique<HittableList>(std::move(list));
    } else {
        scene_desc.width = cli_width;
        scene_desc.height = cli_height;
        scene_desc.samples = cli_samples;
        scene_desc.max_depth = cli_max_depth;
        world = std::make_unique<HittableList>(build_default_scene(materials));
    }

    if (!options.obj_path.empty()) {
        Mesh mesh;
        std::string error;
        if (!load_obj(options.obj_path, mesh, error)) {
            std::cerr << error << '\n';
            return 1;
        }

        materials.push_back(std::make_unique<Lambertian>(Color{0.8, 0.8, 0.85}));
        auto mesh_bvh = build_bvh_from_mesh(mesh, materials.back().get());

        auto combined = std::make_unique<HittableList>();
        combined->add(std::move(world));
        combined->add(std::move(mesh_bvh));
        world = std::move(combined);
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

    const int thread_count = options.threads > 0
        ? options.threads
        : static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));

    const auto start = std::chrono::steady_clock::now();
    std::atomic<int> scanlines_remaining{scene_desc.height};

    auto render_rows = [&](int y_begin, int y_end) {
        for (int y = y_begin; y < y_end; ++y) {
            for (int x = 0; x < scene_desc.width; ++x) {
                Color pixel{0.0, 0.0, 0.0};
                for (int sample = 0; sample < scene_desc.samples; ++sample) {
                    const double u = (x + random_double()) / (scene_desc.width - 1);
                    const double v = (y + random_double()) / (scene_desc.height - 1);
                    Ray ray = camera.get_ray(u, v);
                    pixel += ray_color(ray, *world, scene_desc.max_depth);
                }
                pixel /= static_cast<double>(scene_desc.samples);
                framebuffer.set_pixel(x, scene_desc.height - 1 - y, pixel);
            }

            const int remaining = --scanlines_remaining;
            if (remaining % 16 == 0) {
                std::cerr << "\rScanlines remaining: " << remaining << ' ' << std::flush;
            }
        }
    };

    std::vector<std::thread> threads;
    const int rows_per_thread = (scene_desc.height + thread_count - 1) / thread_count;
    for (int t = 0; t < thread_count; ++t) {
        const int y_begin = t * rows_per_thread;
        const int y_end = std::min(scene_desc.height, y_begin + rows_per_thread);
        if (y_begin >= y_end) {
            continue;
        }
        threads.emplace_back(render_rows, y_begin, y_end);
    }

    if (window) {
        while (scanlines_remaining > 0 && !window->should_close()) {
            window->poll_events();
            window->blit(framebuffer);
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
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
    std::cout << "Wrote " << options.output << " in " << ms << " ms using " << thread_count << " threads\n";
    return 0;
}
