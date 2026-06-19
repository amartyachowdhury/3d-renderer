#pragma once

#include "renderer/math/vec3.h"

namespace renderer {

struct Vec4 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double w = 1.0;

    Vec4() = default;
    Vec4(double x, double y, double z, double w) : x(x), y(y), z(z), w(w) {}
    Vec4(const Vec3& v, double w) : x(v.x), y(v.y), z(v.z), w(w) {}

    Vec3 xyz() const { return {x, y, z}; }
    Vec3 perspective_divide() const {
        if (std::fabs(w) < 1e-8) return xyz();
        return {x / w, y / w, z / w};
    }
};

}  // namespace renderer
