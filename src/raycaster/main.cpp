#include "renderer/core/cli.h"
#include "renderer/core/framebuffer.h"
#include "renderer/core/texture.h"
#include "renderer/math/constants.h"
#include "renderer/platform/sdl_window.h"
#include "renderer/raycaster/map.h"
#include "renderer/raycaster/raycaster_engine.h"

#include <SDL.h>
#include <iostream>
#include <string>
#include <vector>

namespace {

void print_help(const char* program) {
    renderer::print_usage(
        program,
        "[options]\n"
        "  --map PATH         Tile map file\n"
        "  --spawn PATH       Player and sprite spawn file\n"
        "  --textures PREFIX  Directory containing wall1.png ... sprite.png\n"
        "  --output PATH      Output image (.png or .ppm)\n"
        "  --capture-mouse    Lock cursor for mouse look\n"
        "  --dump-ppm         Render one frame and exit\n"
        "  --help             Show this help");
}

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
    const char* names[] = {"wall1.png", "wall2.png", "wall3.png", "sprite.png"};
    for (const char* name : names) {
        renderer::Texture texture;
        if (!renderer::load_texture(prefix + name, texture, error)) {
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
    std::string spawn_path = "assets/maps/level1.spawn";
    std::string texture_prefix = "assets/textures/";
    bool dump_only = false;
    bool capture_mouse = false;
    bool show_help = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            show_help = true;
        } else if (arg == "--output" && i + 1 < argc) {
            output = argv[++i];
        } else if (arg == "--map" && i + 1 < argc) {
            map_path = argv[++i];
        } else if (arg == "--spawn" && i + 1 < argc) {
            spawn_path = argv[++i];
        } else if (arg == "--textures" && i + 1 < argc) {
            texture_prefix = argv[++i];
            if (!texture_prefix.empty() && texture_prefix.back() != '/') {
                texture_prefix += '/';
            }
        } else if (arg == "--capture-mouse") {
            capture_mouse = true;
        } else if (arg == "--dump-ppm") {
            dump_only = true;
        }
    }

    if (show_help) {
        print_help(argv[0]);
        return 0;
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

    SpawnData spawn;
    if (!load_spawn(spawn_path, spawn, error)) {
        std::cerr << error << ", using default spawn\n";
        spawn.sprites = {
            {4.5, 3.5, 3},
            {8.5, 8.5, 3},
            {12.5, 5.5, 3},
            {6.5, 11.5, 3},
        };
    }
    engine.set_sprites(std::move(spawn.sprites));

    Player player = spawn.player;
    Framebuffer framebuffer(settings.width, settings.height);

    if (dump_only) {
        engine.render(player, framebuffer);
        engine.render_minimap(player, framebuffer);
        if (!framebuffer.write_image(output)) {
            std::cerr << "Failed to write " << output << '\n';
            return 1;
        }
        std::cout << "Wrote " << output << '\n';
        return 0;
    }

    SdlWindow window("Raycaster", settings.width, settings.height);
    if (capture_mouse) {
        window.set_relative_mouse_mode(true);
    }

    const double move_speed = 0.06;
    const double rot_speed = 0.04;
    const double mouse_sensitivity = 0.003;

    while (window.is_open() && !window.should_close()) {
        window.poll_events();

        if (window.mouse_delta_x() != 0 || window.mouse_delta_y() != 0) {
            player.angle += window.mouse_delta_x() * mouse_sensitivity;
            window.clear_mouse_delta();
        }

        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        const double forward_x = std::cos(player.angle);
        const double forward_y = std::sin(player.angle);
        const double strafe_x = std::cos(player.angle + kPi / 2.0);
        const double strafe_y = std::sin(player.angle + kPi / 2.0);

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
    }

    if (!framebuffer.write_image(output)) {
        std::cerr << "Failed to write " << output << '\n';
        return 1;
    }

    std::cout << "Wrote " << output << '\n';
    return 0;
}
