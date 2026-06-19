#include "renderer/core/cli.h"
#include "renderer/core/framebuffer.h"
#include "renderer/math/constants.h"
#include "renderer/platform/sdl_window.h"
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

namespace {

void print_help(const char* program) {
    renderer::print_usage(
        program,
        "[options]\n"
        "  --scene PATH       Scene description file\n"
        "  --obj PATH         Add OBJ mesh via BVH\n"
        "  --output PATH      Output image (.png or .ppm)\n"
        "  --preview          Live SDL preview while rendering\n"
        "  --dump-ppm         Render and exit without preview loop\n"
        "  --width N          Image width\n"
        "  --height N         Image height\n"
        "  --samples N        Samples per pixel\n"
        "  --max-depth N      Maximum ray bounce depth\n"
        "  --threads N        Render thread count (0 = auto)\n"
        "  --camera-yaw DEG   Orbit camera yaw in degrees\n"
        "  --help             Show this help");
}

struct Options {
    std::string output = "output.ppm";
    std::string scene_path;
    std::string obj_path;
    bool preview = false;
    bool dump_only = false;
    bool show_help = false;
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
        if (arg == "--help") {
            options.show_help = true;
        } else if (arg == "--output" && i + 1 < argc) {
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
    if (options.show_help) {
        print_help(argv[0]);
        return 0;
    }

    SceneDescription scene_desc;
    const int cli_width = options.width;
    const int cli_height = options.height;
    const int cli_samples = options.samples;
    const int cli_max_depth = options.max_depth;

    std::vector<std::unique_ptr<Material>> materials;
    std::vector<Texture> scene_textures;
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
        build_scene(scene_desc, list, materials, scene_textures);
        world = build_bvh_world(std::move(list));
    } else {
        scene_desc.width = cli_width;
        scene_desc.height = cli_height;
        scene_desc.samples = cli_samples;
        scene_desc.max_depth = cli_max_depth;
        world = build_bvh_world(build_default_scene(materials));
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

        HittableList combined;
        combined.add(std::move(world));
        combined.add(std::move(mesh_bvh));
        world = build_bvh_world(std::move(combined));
    }

    const double aspect = static_cast<double>(scene_desc.width) / static_cast<double>(scene_desc.height);
    const double yaw = options.camera_yaw * kPi / 180.0;
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

    const bool use_preview = options.preview && !options.dump_only;
    const int thread_count = use_preview
        ? 1
        : options.dump_only
              ? 1
              : (options.threads > 0
                    ? options.threads
                    : static_cast<int>(std::max(1u, std::thread::hardware_concurrency())));

    const auto start = std::chrono::steady_clock::now();
    std::atomic<int> tiles_remaining{0};

    constexpr int kTileSize = 32;
    struct Tile {
        int x0;
        int y0;
        int x1;
        int y1;
    };
    std::vector<Tile> tiles;
    for (int y = 0; y < scene_desc.height; y += kTileSize) {
        for (int x = 0; x < scene_desc.width; x += kTileSize) {
            tiles.push_back({
                x,
                y,
                std::min(x + kTileSize, scene_desc.width),
                std::min(y + kTileSize, scene_desc.height),
            });
        }
    }
    tiles_remaining = static_cast<int>(tiles.size());

    auto render_tile = [&](const Tile& tile) {
        for (int y = tile.y0; y < tile.y1; ++y) {
            for (int x = tile.x0; x < tile.x1; ++x) {
                if (options.dump_only) {
                    seed_random(42u + static_cast<uint32_t>(y) * scene_desc.width + static_cast<uint32_t>(x));
                }
                Color pixel{0.0, 0.0, 0.0};
                for (int sample = 0; sample < scene_desc.samples; ++sample) {
                    const double jitter_x = scene_desc.samples > 1 ? random_double() : 0.0;
                    const double jitter_y = scene_desc.samples > 1 ? random_double() : 0.0;
                    const double u = (x + jitter_x) / (scene_desc.width - 1);
                    const double v = (y + jitter_y) / (scene_desc.height - 1);
                    Ray ray = camera.get_ray(u, v);
                    pixel += ray_color(ray, *world, scene_desc.max_depth);
                }
                pixel /= static_cast<double>(scene_desc.samples);
                framebuffer.set_pixel(x, scene_desc.height - 1 - y, pixel);
            }
        }

        const int remaining = --tiles_remaining;
        if (remaining % 16 == 0) {
            std::cerr << "\rTiles remaining: " << remaining << ' ' << std::flush;
        }
    };

    std::atomic<int> next_tile{0};
    auto worker = [&]() {
        while (true) {
            const int index = next_tile.fetch_add(1);
            if (index >= static_cast<int>(tiles.size())) {
                break;
            }
            render_tile(tiles[static_cast<size_t>(index)]);
        }
    };

    std::vector<std::thread> threads;
    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back(worker);
    }

    if (window) {
        while (tiles_remaining > 0 && !window->should_close()) {
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

    if (!framebuffer.write_image(options.output)) {
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
