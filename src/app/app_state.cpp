#include "renderer/app/app_state.h"

namespace renderer {

const char* renderer_mode_name(RendererMode mode) {
    switch (mode) {
        case RendererMode::Raytracer:
            return "Ray Tracer";
        case RendererMode::Rasterizer:
            return "Rasterizer";
        case RendererMode::Raycaster:
            return "Raycaster";
    }
    return "Unknown";
}

}  // namespace renderer
