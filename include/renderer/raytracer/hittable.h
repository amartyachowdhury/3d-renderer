#pragma once

#include "renderer/math/ray.h"
#include "renderer/math/vec3.h"

#include <cmath>
#include <limits>
#include <memory>
#include <vector>

namespace renderer {

struct HitRecord {
    Point3 point;
    Vec3 normal;
    double t = 0.0;
    bool front_face = true;
    class Material* material = nullptr;

    void set_face_normal(const Ray& ray, const Vec3& outward_normal) {
        front_face = ray.direction.dot(outward_normal) < 0.0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class Material {
public:
    virtual ~Material() = default;
    virtual bool scatter(const Ray& ray_in, const HitRecord& hit, Color& attenuation, Ray& scattered) const = 0;
};

class Hittable {
public:
    virtual ~Hittable() = default;
    virtual bool hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const = 0;
};

class Sphere : public Hittable {
public:
    Sphere(Point3 center, double radius, Material* material)
        : center_(center), radius_(radius), material_(material) {}

    bool hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const override {
        Vec3 oc = ray.origin - center_;
        double a = ray.direction.length_squared();
        double half_b = oc.dot(ray.direction);
        double c = oc.length_squared() - radius_ * radius_;
        double discriminant = half_b * half_b - a * c;

        if (discriminant < 0.0) {
            return false;
        }

        double sqrtd = std::sqrt(discriminant);
        double root = (-half_b - sqrtd) / a;
        if (root < t_min || root > t_max) {
            root = (-half_b + sqrtd) / a;
            if (root < t_min || root > t_max) {
                return false;
            }
        }

        record.t = root;
        record.point = ray.at(record.t);
        Vec3 outward_normal = (record.point - center_) / radius_;
        record.set_face_normal(ray, outward_normal);
        record.material = material_;
        return true;
    }

private:
    Point3 center_;
    double radius_;
    Material* material_;
};

class HittableList : public Hittable {
public:
    void add(std::unique_ptr<Hittable> object) { objects_.push_back(std::move(object)); }

    bool hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const override {
        HitRecord temp;
        bool hit_anything = false;
        double closest = t_max;

        for (const auto& object : objects_) {
            if (object->hit(ray, t_min, closest, temp)) {
                hit_anything = true;
                closest = temp.t;
                record = temp;
            }
        }

        return hit_anything;
    }

private:
    std::vector<std::unique_ptr<Hittable>> objects_;
};

}  // namespace renderer
