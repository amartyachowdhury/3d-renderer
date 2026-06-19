#pragma once

#include "renderer/core/framebuffer.h"
#include "renderer/raycaster/map.h"

namespace renderer {

struct Player {
    double x = 2.5;
    double y = 2.5;
    double angle = 0.0;
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

    void render(const Player& player, Framebuffer& framebuffer) const;
    void render_minimap(const Player& player, Framebuffer& framebuffer) const;

private:
    const Map& map_;
    RaycasterSettings settings_;
};

}  // namespace renderer
