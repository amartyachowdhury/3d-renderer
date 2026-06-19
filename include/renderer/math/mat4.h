#pragma once

#include "renderer/math/vec3.h"
#include "renderer/math/vec4.h"

#include <array>
#include <cmath>

namespace renderer {

struct Mat4 {
    std::array<double, 16> m{};

    static Mat4 identity() {
        Mat4 result;
        result.m[0] = result.m[5] = result.m[10] = result.m[15] = 1.0;
        return result;
    }

    double& operator()(int row, int col) { return m[col * 4 + row]; }
    double operator()(int row, int col) const { return m[col * 4 + row]; }

    static Mat4 multiply(const Mat4& a, const Mat4& b) {
        Mat4 result;
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                result(row, col) = a(row, 0) * b(0, col) + a(row, 1) * b(1, col) +
                                   a(row, 2) * b(2, col) + a(row, 3) * b(3, col);
            }
        }
        return result;
    }

    Vec4 transform_point(const Vec4& v) const {
        return {
            (*this)(0, 0) * v.x + (*this)(0, 1) * v.y + (*this)(0, 2) * v.z + (*this)(0, 3) * v.w,
            (*this)(1, 0) * v.x + (*this)(1, 1) * v.y + (*this)(1, 2) * v.z + (*this)(1, 3) * v.w,
            (*this)(2, 0) * v.x + (*this)(2, 1) * v.y + (*this)(2, 2) * v.z + (*this)(2, 3) * v.w,
            (*this)(3, 0) * v.x + (*this)(3, 1) * v.y + (*this)(3, 2) * v.z + (*this)(3, 3) * v.w,
        };
    }

    static Mat4 translation(const Vec3& t) {
        Mat4 result = identity();
        result(0, 3) = t.x;
        result(1, 3) = t.y;
        result(2, 3) = t.z;
        return result;
    }

    static Mat4 rotation_y(double radians) {
        Mat4 result = identity();
        const double c = std::cos(radians);
        const double s = std::sin(radians);
        result(0, 0) = c;
        result(0, 2) = s;
        result(2, 0) = -s;
        result(2, 2) = c;
        return result;
    }

    static Mat4 look_at(const Point3& eye, const Point3& target, const Vec3& up) {
        const Vec3 f = (target - eye).normalized();
        const Vec3 r = f.cross(up).normalized();
        const Vec3 u = r.cross(f);

        Mat4 result = identity();
        result(0, 0) = r.x;
        result(1, 0) = r.y;
        result(2, 0) = r.z;
        result(0, 1) = u.x;
        result(1, 1) = u.y;
        result(2, 1) = u.z;
        result(0, 2) = -f.x;
        result(1, 2) = -f.y;
        result(2, 2) = -f.z;
        result(0, 3) = -r.dot(eye);
        result(1, 3) = -u.dot(eye);
        result(2, 3) = f.dot(eye);
        return result;
    }

    static Mat4 perspective(double fov_y_radians, double aspect, double near_plane, double far_plane) {
        Mat4 result{};
        const double tan_half = std::tan(fov_y_radians / 2.0);
        result(0, 0) = 1.0 / (aspect * tan_half);
        result(1, 1) = 1.0 / tan_half;
        result(2, 2) = -(far_plane + near_plane) / (far_plane - near_plane);
        result(2, 3) = -(2.0 * far_plane * near_plane) / (far_plane - near_plane);
        result(3, 2) = -1.0;
        return result;
    }
};

inline Mat4 operator*(const Mat4& a, const Mat4& b) { return Mat4::multiply(a, b); }

}  // namespace renderer
