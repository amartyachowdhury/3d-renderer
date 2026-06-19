#pragma once

#include "renderer/raytracer/hittable.h"

#include <cmath>
#include <memory>
#include <random>

namespace renderer {

inline double random_double() {
    static thread_local std::mt19937 generator{std::random_device{}()};
    static thread_local std::uniform_real_distribution<double> distribution(0.0, 1.0);
    return distribution(generator);
}

inline double random_double(double min, double max) {
    return min + (max - min) * random_double();
}

inline Vec3 random_in_unit_sphere() {
    while (true) {
        Vec3 p{random_double(-1, 1), random_double(-1, 1), random_double(-1, 1)};
        if (p.length_squared() >= 1.0) {
            continue;
        }
        return p;
    }
}

inline Vec3 random_unit_vector() { return random_in_unit_sphere().normalized(); }

inline Vec3 random_in_hemisphere(const Vec3& normal) {
    Vec3 in_unit_sphere = random_in_unit_sphere();
    if (in_unit_sphere.dot(normal) > 0.0) {
        return in_unit_sphere;
    }
    return -in_unit_sphere;
}

inline Vec3 random_in_unit_disk() {
    while (true) {
        Vec3 p{random_double(-1, 1), random_double(-1, 1), 0.0};
        if (p.length_squared() >= 1.0) {
            continue;
        }
        return p;
    }
}

inline double reflectance(double cosine, double ref_idx) {
    double r0 = (1.0 - ref_idx) / (1.0 + ref_idx);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * std::pow((1.0 - cosine), 5.0);
}

class Lambertian : public Material {
public:
    explicit Lambertian(const Color& albedo) : albedo_(albedo) {}

    bool scatter(const Ray&, const HitRecord& hit, Color& attenuation, Ray& scattered) const override {
        Vec3 scatter_direction = hit.normal + random_unit_vector();
        if (scatter_direction.near_zero()) {
            scatter_direction = hit.normal;
        }
        scattered = Ray(hit.point, scatter_direction);
        attenuation = albedo_;
        return true;
    }

private:
    Color albedo_;
};

class Metal : public Material {
public:
    Metal(const Color& albedo, double fuzz) : albedo_(albedo), fuzz_(fuzz < 1.0 ? fuzz : 1.0) {}

    bool scatter(const Ray& ray_in, const HitRecord& hit, Color& attenuation, Ray& scattered) const override {
        Vec3 reflected = Vec3::reflect(ray_in.direction.normalized(), hit.normal);
        scattered = Ray(hit.point, reflected + fuzz_ * random_in_unit_sphere());
        attenuation = albedo_;
        return scattered.direction.dot(hit.normal) > 0.0;
    }

private:
    Color albedo_;
    double fuzz_;
};

class Dielectric : public Material {
public:
    explicit Dielectric(double refraction_index) : refraction_index_(refraction_index) {}

    bool scatter(const Ray& ray_in, const HitRecord& hit, Color& attenuation, Ray& scattered) const override {
        attenuation = {1.0, 1.0, 1.0};
        double refraction_ratio = hit.front_face ? (1.0 / refraction_index_) : refraction_index_;

        Vec3 unit_direction = ray_in.direction.normalized();
        double cos_theta = std::fmin(-unit_direction.dot(hit.normal), 1.0);
        double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

        bool cannot_refract = refraction_ratio * sin_theta > 1.0;
        Vec3 direction;

        if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_double()) {
            direction = Vec3::reflect(unit_direction, hit.normal);
        } else {
            direction = Vec3::refract(unit_direction, hit.normal, refraction_ratio);
        }

        scattered = Ray(hit.point, direction);
        return true;
    }

private:
    double refraction_index_;
};

class Camera {
public:
    Camera(
        Point3 look_from,
        Point3 look_at,
        Vec3 vup,
        double vertical_fov,
        double aspect_ratio,
        double aperture,
        double focus_dist)
        : origin_(look_from), vertical_fov_(vertical_fov), aspect_ratio_(aspect_ratio) {
        const double theta = vertical_fov * M_PI / 180.0;
        const double h = std::tan(theta / 2.0);
        const double viewport_height = 2.0 * h;
        const double viewport_width = aspect_ratio * viewport_height;

        w_ = (look_from - look_at).normalized();
        u_ = vup.cross(w_).normalized();
        v_ = w_.cross(u_);

        horizontal_ = focus_dist * viewport_width * u_;
        vertical_ = focus_dist * viewport_height * v_;
        lower_left_corner_ = origin_ - focus_dist * w_ - horizontal_ / 2.0 - vertical_ / 2.0;

        lens_radius_ = aperture / 2.0;
    }

    Ray get_ray(double s, double t) const {
        Vec3 offset = lens_radius_ > 0.0 ? lens_radius_ * random_in_unit_disk() : Vec3{};
        Point3 origin = origin_ + offset;
        Point3 target = lower_left_corner_ + s * horizontal_ + t * vertical_;
        return Ray(origin, target - origin);
    }

private:
    Point3 origin_;
    Point3 lower_left_corner_;
    Vec3 horizontal_;
    Vec3 vertical_;
    Vec3 u_;
    Vec3 v_;
    Vec3 w_;
    double lens_radius_ = 0.0;
    double vertical_fov_;
    double aspect_ratio_;
};

inline Color ray_color(const Ray& ray, const Hittable& world, int depth) {
    if (depth <= 0) {
        return {0.0, 0.0, 0.0};
    }

    HitRecord record;
    if (world.hit(ray, 0.001, std::numeric_limits<double>::infinity(), record)) {
        Ray scattered;
        Color attenuation;
        if (record.material && record.material->scatter(ray, record, attenuation, scattered)) {
            return attenuation * ray_color(scattered, world, depth - 1);
        }
        return {0.0, 0.0, 0.0};
    }

    Vec3 unit_direction = ray.direction.normalized();
    double t = 0.5 * (unit_direction.y + 1.0);
    return (1.0 - t) * Color{1.0, 1.0, 1.0} + t * Color{0.5, 0.7, 1.0};
}

}  // namespace renderer

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
