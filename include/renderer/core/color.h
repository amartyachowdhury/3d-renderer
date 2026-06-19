#pragma once

#include "renderer/math/vec3.h"

#include <algorithm>
#include <cmath>

namespace renderer {

inline double linear_to_gamma(double linear) {
    return linear > 0.0 ? std::sqrt(linear) : 0.0;
}

inline Color clamp_color(const Color& c) {
    return {
        std::clamp(c.x, 0.0, 1.0),
        std::clamp(c.y, 0.0, 1.0),
        std::clamp(c.z, 0.0, 1.0),
    };
}

inline Color to_display_color(const Color& linear) {
    return {
        linear_to_gamma(linear.x),
        linear_to_gamma(linear.y),
        linear_to_gamma(linear.z),
    };
}

}  // namespace renderer
