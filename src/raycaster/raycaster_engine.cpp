#include "renderer/raycaster/raycaster_engine.h"

#include <algorithm>
#include <cmath>
#include <vector>

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

struct ColumnHit {
    int draw_start = 0;
    int draw_end = 0;
    double perp_dist = 1.0;
    int tile = 0;
    bool side_hit = false;
    double wall_x = 0.0;
};

}  // namespace

RaycasterEngine::RaycasterEngine(const Map& map, const RaycasterSettings& settings)
    : map_(map), settings_(settings) {}

void RaycasterEngine::set_wall_textures(std::vector<Texture> textures) {
    wall_textures_ = std::move(textures);
}

void RaycasterEngine::set_sprites(std::vector<Sprite> sprites) {
    sprites_ = std::move(sprites);
}

Color RaycasterEngine::sample_wall(int tile, bool side_hit, double wall_x, int tex_y) const {
    const int index = tile - 1;
    if (index < 0 || index >= static_cast<int>(wall_textures_.size()) || wall_textures_[index].rgba.empty()) {
        return wall_color(tile, side_hit);
    }

    const Texture& texture = wall_textures_[index];
    const int x = std::clamp(static_cast<int>(wall_x * texture.width), 0, texture.width - 1);
    const int y = std::clamp(tex_y, 0, texture.height - 1);
    const int pixel = (y * texture.width + x) * 4;
    Color color{
        texture.rgba[pixel + 0] / 255.0,
        texture.rgba[pixel + 1] / 255.0,
        texture.rgba[pixel + 2] / 255.0,
    };
    if (side_hit) {
        color *= 0.75;
    }
    return color;
}

void RaycasterEngine::render(const Player& player, Framebuffer& framebuffer) const {
    const int half_height = settings_.height / 2;
    std::vector<ColumnHit> columns(settings_.width);
    std::vector<double> z_buffer(settings_.width, 0.0);

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
        z_buffer[x] = perp_dist;

        double wall_x = side_hit ? player.x + perp_dist * ray_dir_x : player.y + perp_dist * ray_dir_y;
        wall_x -= std::floor(wall_x);

        const int line_height = static_cast<int>(settings_.height / perp_dist);
        const int draw_start = std::max(0, -line_height / 2 + half_height);
        const int draw_end = std::min(settings_.height - 1, line_height / 2 + half_height);

        columns[x] = {draw_start, draw_end, perp_dist, tile, side_hit, wall_x};

        const double shade = std::clamp(1.0 / (1.0 + perp_dist * 0.35), 0.2, 1.0);

        for (int y = 0; y < settings_.height; ++y) {
            Color color;
            if (y <= draw_start) {
                color = {0.45, 0.55, 0.75};
            } else if (y >= draw_end) {
                color = {0.18, 0.18, 0.2};
            } else {
                const int tex_y = static_cast<int>(((y - draw_start) * 64) / std::max(1, draw_end - draw_start));
                color = sample_wall(tile, side_hit, wall_x, tex_y) * shade;
            }
            framebuffer.set_pixel(x, y, color);
        }
    }

    std::vector<Sprite> sorted = sprites_;
    std::sort(sorted.begin(), sorted.end(), [&](const Sprite& a, const Sprite& b) {
        const double da = (player.x - a.x) * (player.x - a.x) + (player.y - a.y) * (player.y - a.y);
        const double db = (player.x - b.x) * (player.x - b.x) + (player.y - b.y) * (player.y - b.y);
        return da > db;
    });

    for (const Sprite& sprite : sorted) {
        const double sprite_x = sprite.x - player.x;
        const double sprite_y = sprite.y - player.y;

        const double inv_det = 1.0 / (std::cos(player.angle) * std::sin(player.angle) - std::sin(player.angle) * std::cos(player.angle));
        const double transform_x = inv_det * (std::sin(player.angle) * sprite_y - std::cos(player.angle) * sprite_x);
        const double transform_y = inv_det * (-std::sin(player.angle) * sprite_x + std::cos(player.angle) * sprite_y);

        if (transform_y <= 0.1) {
            continue;
        }

        const int sprite_screen_x = static_cast<int>((settings_.width / 2.0) * (1.0 + transform_x / transform_y));
        const int sprite_height = std::abs(static_cast<int>(settings_.height / transform_y));
        const int draw_start_y = std::max(0, -sprite_height / 2 + settings_.height / 2);
        const int draw_end_y = std::min(settings_.height - 1, sprite_height / 2 + settings_.height / 2);
        const int sprite_width = std::abs(static_cast<int>(settings_.height / transform_y));
        const int draw_start_x = std::max(0, -sprite_width / 2 + sprite_screen_x);
        const int draw_end_x = std::min(settings_.width - 1, sprite_width / 2 + sprite_screen_x);

        if (sprite.texture_id < 0 || sprite.texture_id >= static_cast<int>(wall_textures_.size())) {
            continue;
        }
        const Texture& texture = wall_textures_[sprite.texture_id];

        for (int stripe = draw_start_x; stripe < draw_end_x; ++stripe) {
            if (transform_y >= z_buffer[stripe]) {
                continue;
            }

            const int tex_x = static_cast<int>(256.0 * (stripe - (-sprite_width / 2 + sprite_screen_x)) / sprite_width) & 255;
            for (int y = draw_start_y; y < draw_end_y; ++y) {
                const int d = y * 256 - settings_.height * 128 + sprite_height * 128;
                const int tex_y = ((d * texture.height) / sprite_height) / 256;
                if (tex_x < 0 || tex_x >= texture.width || tex_y < 0 || tex_y >= texture.height) {
                    continue;
                }
                const int index = (tex_y * texture.width + tex_x) * 4;
                Color color{
                    texture.rgba[index + 0] / 255.0,
                    texture.rgba[index + 1] / 255.0,
                    texture.rgba[index + 2] / 255.0,
                };
                const double shade = std::clamp(1.0 / (1.0 + transform_y * 0.35), 0.2, 1.0);
                framebuffer.set_pixel(stripe, y, color * shade);
            }
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

    for (const Sprite& sprite : sprites_) {
        const int sx = offset_x + static_cast<int>(sprite.x * scale);
        const int sy = offset_y + static_cast<int>(sprite.y * scale);
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                framebuffer.set_pixel(sx + dx, sy + dy, {0.2, 0.9, 0.3});
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

    for (int t = 0; t < 10; ++t) {
        framebuffer.set_pixel(
            px + static_cast<int>(std::cos(player.angle) * t),
            py + static_cast<int>(std::sin(player.angle) * t),
            {1.0, 0.8, 0.2});
    }
}

}  // namespace renderer
