#pragma once

#include <cmath>
#include <iostream>

namespace renderer {

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    Vec3() = default;
    Vec3(double x, double y, double z) : x(x), y(y), z(z) {}

    Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3 operator+(const Vec3& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Vec3 operator-(const Vec3& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Vec3 operator*(const Vec3& other) const { return {x * other.x, y * other.y, z * other.z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(double s) const { return {x / s, y / s, z / s}; }

    Vec3& operator+=(const Vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vec3& operator*=(double s) {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    Vec3& operator/=(double s) {
        return *this *= (1.0 / s);
    }

    double dot(const Vec3& other) const { return x * other.x + y * other.y + z * other.z; }

    Vec3 cross(const Vec3& other) const {
        return {
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x,
        };
    }

    double length() const { return std::sqrt(dot(*this)); }
    double length_squared() const { return dot(*this); }

    Vec3 normalized() const {
        const double len = length();
        return len > 0.0 ? *this / len : Vec3{};
    }

    bool near_zero() const {
        const auto s = 1e-8;
        return std::fabs(x) < s && std::fabs(y) < s && std::fabs(z) < s;
    }

    static Vec3 reflect(const Vec3& v, const Vec3& n) {
        return v - n * (2.0 * v.dot(n));
    }

    static Vec3 refract(const Vec3& uv, const Vec3& n, double eta_ratio) {
        const double cos_theta = std::fmin(-uv.dot(n), 1.0);
        Vec3 r_out_perp = (uv + n * cos_theta) * eta_ratio;
        Vec3 r_out_parallel = n * (-std::sqrt(std::fabs(1.0 - r_out_perp.length_squared())));
        return r_out_perp + r_out_parallel;
    }

    double operator[](int i) const {
        if (i == 0) return x;
        if (i == 1) return y;
        return z;
    }

    double& operator[](int i) {
        if (i == 0) return x;
        if (i == 1) return y;
        return z;
    }
};

inline Vec3 operator*(double s, const Vec3& v) { return v * s; }

inline std::ostream& operator<<(std::ostream& out, const Vec3& v) {
    return out << v.x << ' ' << v.y << ' ' << v.z;
}

using Point3 = Vec3;
using Color = Vec3;

}  // namespace renderer
