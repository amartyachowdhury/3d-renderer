#pragma once

#include "renderer/raytracer/aabb.h"
#include "renderer/raytracer/hittable.h"
#include "renderer/raytracer/triangle.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace renderer {

inline bool hittable_compare(const std::unique_ptr<Hittable>& a, const std::unique_ptr<Hittable>& b, int axis) {
    return a->bounding_box().min()[axis] < b->bounding_box().min()[axis];
}

class BVHNode : public Hittable {
public:
    BVHNode() = default;

    BVHNode(std::vector<std::unique_ptr<Hittable>>& objects, int start, int end) {
        int axis = 2;
        const int object_span = end - start;

        if (object_span == 1) {
            left_ = std::move(objects[start]);
        } else if (object_span == 2) {
            left_ = std::move(objects[start]);
            right_ = std::move(objects[start + 1]);
        } else {
            AABB box = objects[start]->bounding_box();
            for (int i = start + 1; i < end; ++i) {
                box = AABB::surrounding_box(box, objects[i]->bounding_box());
            }

            for (int candidate = 0; candidate < 3; ++candidate) {
                if ((box.max()[candidate] - box.min()[candidate]) > (box.max()[axis] - box.min()[axis])) {
                    axis = candidate;
                }
            }

            auto comparator = [axis](const std::unique_ptr<Hittable>& a, const std::unique_ptr<Hittable>& b) {
                return hittable_compare(a, b, axis);
            };

            const int mid = start + object_span / 2;
            std::nth_element(objects.begin() + start, objects.begin() + mid, objects.begin() + end, comparator);
            left_ = std::make_unique<BVHNode>(objects, start, mid);
            right_ = std::make_unique<BVHNode>(objects, mid, end);
        }

        box_ = left_->bounding_box();
        if (right_) {
            box_ = AABB::surrounding_box(box_, right_->bounding_box());
        }
    }

    bool hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const override {
        if (!box_.hit(ray, t_min, t_max)) {
            return false;
        }

        const bool hit_left = left_->hit(ray, t_min, t_max, record);
        if (!right_) {
            return hit_left;
        }

        const bool hit_right = right_->hit(ray, t_min, hit_left ? record.t : t_max, record);
        return hit_left || hit_right;
    }

    AABB bounding_box() const override { return box_; }

private:
    AABB box_;
    std::unique_ptr<Hittable> left_;
    std::unique_ptr<Hittable> right_;
};

inline std::unique_ptr<Hittable> build_bvh(std::vector<std::unique_ptr<Hittable>> objects) {
    if (objects.empty()) {
        return nullptr;
    }
    return std::make_unique<BVHNode>(objects, 0, static_cast<int>(objects.size()));
}

}  // namespace renderer
