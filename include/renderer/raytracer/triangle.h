#pragma once

#include "renderer/raytracer/aabb.h"
#include "renderer/raytracer/hittable.h"

#include <algorithm>

namespace renderer {

class Triangle : public Hittable {
public:
    Triangle(const Point3& v0, const Point3& v1, const Point3& v2, Material* material)
        : v0_(v0), v1_(v1), v2_(v2), material_(material) {
        Point3 min{
            std::min(v0.x, std::min(v1.x, v2.x)),
            std::min(v0.y, std::min(v1.y, v2.y)),
            std::min(v0.z, std::min(v1.z, v2.z)),
        };
        Point3 max{
            std::max(v0.x, std::max(v1.x, v2.x)),
            std::max(v0.y, std::max(v1.y, v2.y)),
            std::max(v0.z, std::max(v1.z, v2.z)),
        };
        bounding_box_ = AABB{min, max};
    }

    bool hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const override {
        const Point3 edge1 = v1_ - v0_;
        const Point3 edge2 = v2_ - v0_;
        const Point3 pvec = ray.direction.cross(edge2);
        const double det = edge1.dot(pvec);

        if (std::fabs(det) < 1e-8) {
            return false;
        }

        const double inv_det = 1.0 / det;
        const Point3 tvec = ray.origin - v0_;
        const double u = tvec.dot(pvec) * inv_det;
        if (u < 0.0 || u > 1.0) {
            return false;
        }

        const Point3 qvec = tvec.cross(edge1);
        const double v = ray.direction.dot(qvec) * inv_det;
        if (v < 0.0 || u + v > 1.0) {
            return false;
        }

        const double t = edge2.dot(qvec) * inv_det;
        if (t < t_min || t > t_max) {
            return false;
        }

        record.t = t;
        record.point = ray.at(t);
        const Vec3 outward = edge1.cross(edge2).normalized();
        record.set_face_normal(ray, outward);
        record.material = material_;
        return true;
    }

    AABB bounding_box() const override {
        return bounding_box_;
    }

private:
    Point3 v0_;
    Point3 v1_;
    Point3 v2_;
    Material* material_;
    AABB bounding_box_;
};

}  // namespace renderer
