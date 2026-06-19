#pragma once

#include "renderer/math/ray.h"
#include "renderer/math/vec3.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace renderer {

class AABB {
public:
    AABB() = default;
    AABB(const Point3& min, const Point3& max) : min_(min), max_(max) {}

    const Point3& min() const { return min_; }
    const Point3& max() const { return max_; }

    bool hit(const Ray& ray, double t_min, double t_max) const {
        for (int axis = 0; axis < 3; ++axis) {
            const double origin = ray.origin[axis];
            const double direction = ray.direction[axis];
            const double inv_dir = direction != 0.0 ? 1.0 / direction : std::numeric_limits<double>::infinity();

            double t0 = (min_[axis] - origin) * inv_dir;
            double t1 = (max_[axis] - origin) * inv_dir;
            if (inv_dir < 0.0) {
                std::swap(t0, t1);
            }

            t_min = t0 > t_min ? t0 : t_min;
            t_max = t1 < t_max ? t1 : t_max;
            if (t_max <= t_min) {
                return false;
            }
        }
        return true;
    }

    static AABB surrounding_box(const AABB& box0, const AABB& box1) {
        Point3 min{
            std::min(box0.min_.x, box1.min_.x),
            std::min(box0.min_.y, box1.min_.y),
            std::min(box0.min_.z, box1.min_.z),
        };
        Point3 max{
            std::max(box0.max_.x, box1.max_.x),
            std::max(box0.max_.y, box1.max_.y),
            std::max(box0.max_.z, box1.max_.z),
        };
        return AABB{min, max};
    }

private:
    Point3 min_;
    Point3 max_;
};

}  // namespace renderer
