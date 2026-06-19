#include "renderer/core/framebuffer.h"
#include "renderer/core/texture.h"
#include "renderer/platform/sdl_window.h"
#include "renderer/raycaster/map.h"
#include "renderer/raycaster/raycaster_engine.h"

#include <SDL.h>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

std::vector<std::string> default_map() {
    return {
        "111111111111",
        "100000000001",
        "101111101001",
        "101000101001",
        "101011101001",
        "100010000001",
        "111111111111",
    };
}

bool can_move(const renderer::Map& map, double x, double y) {
    return map.tile(static_cast<int>(x), static_cast<int>(y)) == 0;
}

std::vector<renderer::Texture> load_wall_textures(const std::string& prefix, std::string& error) {
    std::vector<renderer::Texture> textures;
    const char* names[] = {"wall1.ppm", "wall2.ppm", "wall3.ppm", "sprite.ppm"};
    for (const char* name : names) {
        renderer::Texture texture;
        if (!renderer::load_ppm_texture(prefix + name, texture, error)) {
            return {};
        }
        textures.push_back(std::move(texture));
    }
    return textures;
}

}  // namespace

int main(int argc, char** argv) {
    using namespace renderer;

    std::string output = "raycaster.ppm";
    std::string map_path = "assets/maps/level1.txt";
    std::string texture_prefix = "assets/textures/";
    bool dump_only = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--output" && i + 1 < argc) {
            output = argv[++i];
        } else if (arg == "--map" && i + 1 < argc) {
            map_path = argv[++i];
        } else if (arg == "--textures" && i + 1 < argc) {
            texture_prefix = argv[++i];
            if (!texture_prefix.empty() && texture_prefix.back() != '/') {
                texture_prefix += '/';
            }
        } else if (arg == "--dump-ppm") {
            dump_only = true;
        }
    }

    std::vector<std::string> rows;
    std::string error;
    if (!load_map(map_path, rows, error)) {
        std::cerr << error << ", using built-in map\n";
        rows = default_map();
    }

    Map map(rows);
    RaycasterSettings settings;
    RaycasterEngine engine(map, settings);

    auto wall_textures = load_wall_textures(texture_prefix, error);
    if (wall_textures.empty()) {
        std::cerr << "Texture load failed: " << error << ", using procedural textures\n";
        wall_textures = {
            make_checker_texture(64, 8, {0.8, 0.25, 0.25}, {0.45, 0.12, 0.12}),
            make_checker_texture(64, 8, {0.25, 0.75, 0.35}, {0.12, 0.45, 0.18}),
            make_checker_texture(64, 8, {0.25, 0.45, 0.85}, {0.12, 0.25, 0.55}),
            make_checker_texture(64, 4, {0.9, 0.25, 0.25}, {0.15, 0.15, 0.15}),
        };
    }
    engine.set_wall_textures(std::move(wall_textures));

    engine.set_sprites({
        {4.5, 3.5, 3},
        {8.5, 8.5, 3},
        {12.5, 5.5, 3},
        {6.5, 11.5, 3},
    });

    Player player{2.5, 2.5, 0.0};
    Framebuffer framebuffer(settings.width, settings.height);
    SdlWindow window("Raycaster", settings.width, settings.height);

    const double move_speed = 0.06;
    const double rot_speed = 0.04;

    while (window.is_open() && !window.should_close()) {
        window.poll_events();

        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        const double forward_x = std::cos(player.angle);
        const double forward_y = std::sin(player.angle);
        const double strafe_x = std::cos(player.angle + M_PI / 2.0);
        const double strafe_y = std::sin(player.angle + M_PI / 2.0);

        if (keys[SDL_SCANCODE_W]) {
            double nx = player.x + forward_x * move_speed;
            double ny = player.y + forward_y * move_speed;
            if (can_move(map, nx, player.y)) player.x = nx;
            if (can_move(map, player.x, ny)) player.y = ny;
        }
        if (keys[SDL_SCANCODE_S]) {
            double nx = player.x - forward_x * move_speed;
            double ny = player.y - forward_y * move_speed;
            if (can_move(map, nx, player.y)) player.x = nx;
            if (can_move(map, player.x, ny)) player.y = ny;
        }
        if (keys[SDL_SCANCODE_A]) {
            double nx = player.x - strafe_x * move_speed;
            double ny = player.y - strafe_y * move_speed;
            if (can_move(map, nx, player.y)) player.x = nx;
            if (can_move(map, player.x, ny)) player.y = ny;
        }
        if (keys[SDL_SCANCODE_D]) {
            double nx = player.x + strafe_x * move_speed;
            double ny = player.y + strafe_y * move_speed;
            if (can_move(map, nx, player.y)) player.x = nx;
            if (can_move(map, player.x, ny)) player.y = ny;
        }
        if (keys[SDL_SCANCODE_LEFT]) {
            player.angle -= rot_speed;
        }
        if (keys[SDL_SCANCODE_RIGHT]) {
            player.angle += rot_speed;
        }

        engine.render(player, framebuffer);
        engine.render_minimap(player, framebuffer);
        window.blit(framebuffer);

        if (dump_only) {
            break;
        }
    }

    framebuffer.write_ppm(output);
    std::cout << "Wrote " << output << '\n';
    return 0;
}
