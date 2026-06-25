#pragma once

#include <string>

namespace renderer {

enum class RendererMode {
    Raytracer,
    Rasterizer,
    Raycaster,
};

struct AppState {
    RendererMode mode = RendererMode::Rasterizer;
    std::string status_message = "Ready";
    bool wireframe = false;
    int debug_mode_index = 0;
};

const char* renderer_mode_name(RendererMode mode);

}  // namespace renderer
