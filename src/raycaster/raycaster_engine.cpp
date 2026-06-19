#include "renderer/raycaster/raycaster_engine.h"

#include <algorithm>
#include <cmath>

namespace renderer {

namespace {

Color wall_color(int tile_id, bool side_hit) {
    Color base;
    switch (tile_id) {
        case 1: base = {0.75, 0.25, 0.25}; break;
        case 2: base = {0.25, 0.75, 0.35}; break;
        case 3: base = {0.25, 0.45, 0.85}; break;
        case 4: base = {0.85, 0.75, 0.25}; break;
        default: base = {0.65, 0.65, 0.65}; break;
    }
    if (side_hit) {
        base *= 0.75;
    }
    return base;
}

}  // namespace

RaycasterEngine::RaycasterEngine(const Map& map, const RaycasterSettings& settings)
    : map_(map), settings_(settings) {}

void RaycasterEngine::render(const Player& player, Framebuffer& framebuffer) const {
    const int half_height = settings_.height / 2;

    for (int x = 0; x < settings_.width; ++x) {
        const double camera_x = 2.0 * x / static_cast<double>(settings_.width) - 1.0;
        const double ray_angle = player.angle + std::atan(camera_x * std::tan(settings_.fov / 2.0));
        const double ray_dir_x = std::cos(ray_angle);
        const double ray_dir_y = std::sin(ray_angle);

        int map_x = static_cast<int>(player.x);
        int map_y = static_cast<int>(player.y);

        const double delta_dist_x = std::fabs(1.0 / ray_dir_x);
        const double delta_dist_y = std::fabs(1.0 / ray_dir_y);

        int step_x = 0;
        int step_y = 0;
        double side_dist_x = 0.0;
        double side_dist_y = 0.0;

        if (ray_dir_x < 0.0) {
            step_x = -1;
            side_dist_x = (player.x - map_x) * delta_dist_x;
        } else {
            step_x = 1;
            side_dist_x = (map_x + 1.0 - player.x) * delta_dist_x;
        }

        if (ray_dir_y < 0.0) {
            step_y = -1;
            side_dist_y = (player.y - map_y) * delta_dist_y;
        } else {
            step_y = 1;
            side_dist_y = (map_y + 1.0 - player.y) * delta_dist_y;
        }

        bool hit = false;
        bool side_hit = false;
        int tile = 0;

        while (!hit) {
            if (side_dist_x < side_dist_y) {
                side_dist_x += delta_dist_x;
                map_x += step_x;
                side_hit = false;
            } else {
                side_dist_y += delta_dist_y;
                map_y += step_y;
                side_hit = true;
            }

            tile = map_.tile(map_x, map_y);
            if (tile > 0) {
                hit = true;
            }
        }

        double perp_dist = side_hit ? (map_y - player.y + (1 - step_y) / 2.0) / ray_dir_y
                                    : (map_x - player.x + (1 - step_x) / 2.0) / ray_dir_x;
        perp_dist = std::fabs(perp_dist);

        const int line_height = static_cast<int>(settings_.height / perp_dist);
        int draw_start = std::max(0, -line_height / 2 + half_height);
        int draw_end = std::min(settings_.height - 1, line_height / 2 + half_height);

        const Color wall = wall_color(tile, side_hit);
        const double shade = std::clamp(1.0 / (1.0 + perp_dist * 0.35), 0.2, 1.0);

        for (int y = 0; y < settings_.height; ++y) {
            Color color;
            if (y <= draw_start) {
                color = {0.45, 0.55, 0.75};
            } else if (y >= draw_end) {
                color = {0.18, 0.18, 0.2};
            } else {
                color = wall * shade;
            }
            framebuffer.set_pixel(x, y, color);
        }
    }
}

void RaycasterEngine::render_minimap(const Player& player, Framebuffer& framebuffer) const {
    if (!settings_.show_minimap) {
        return;
    }

    const int scale = 8;
    const int offset_x = 10;
    const int offset_y = 10;

    for (int y = 0; y < map_.height(); ++y) {
        for (int x = 0; x < map_.width(); ++x) {
            Color color = map_.tile(x, y) > 0 ? Color{0.35, 0.35, 0.35} : Color{0.12, 0.12, 0.12};
            for (int py = 0; py < scale; ++py) {
                for (int px = 0; px < scale; ++px) {
                    framebuffer.set_pixel(offset_x + x * scale + px, offset_y + y * scale + py, color);
                }
            }
        }
    }

    const int px = offset_x + static_cast<int>(player.x * scale);
    const int py = offset_y + static_cast<int>(player.y * scale);
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            framebuffer.set_pixel(px + dx, py + dy, {1.0, 0.2, 0.2});
        }
    }

    const int lx = px + static_cast<int>(std::cos(player.angle) * 10);
    const int ly = py + static_cast<int>(std::sin(player.angle) * 10);
    for (int t = 0; t < 10; ++t) {
        framebuffer.set_pixel(px + static_cast<int>(std::cos(player.angle) * t), py + static_cast<int>(std::sin(player.angle) * t), {1.0, 0.8, 0.2});
    }
    framebuffer.set_pixel(lx, ly, {1.0, 0.8, 0.2});
}

}  // namespace renderer
