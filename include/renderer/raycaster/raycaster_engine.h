#pragma once

#include "renderer/core/framebuffer.h"
#include "renderer/core/texture.h"
#include "renderer/raycaster/map.h"

#include <vector>

namespace renderer {

struct Player {
    double x = 2.5;
    double y = 2.5;
    double angle = 0.0;
};

struct Sprite {
    double x = 0.0;
    double y = 0.0;
    int texture_id = 0;
};

struct RaycasterSettings {
    int width = 960;
    int height = 540;
    double fov = 66.0 * 3.14159265358979323846 / 180.0;
    bool show_minimap = true;
};

class RaycasterEngine {
public:
    RaycasterEngine(const Map& map, const RaycasterSettings& settings);

    void set_wall_textures(std::vector<Texture> textures);
    void set_sprites(std::vector<Sprite> sprites);

    void render(const Player& player, Framebuffer& framebuffer) const;
    void render_minimap(const Player& player, Framebuffer& framebuffer) const;

private:
    const Map& map_;
    RaycasterSettings settings_;
    std::vector<Texture> wall_textures_;
    std::vector<Sprite> sprites_;

    Color sample_wall(int tile, bool side_hit, double wall_x, int tex_y) const;
};

}  // namespace renderer
