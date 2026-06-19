#pragma once

#include <cmath>

namespace renderer {

struct Vec2 {
    double x = 0.0;
    double y = 0.0;

    Vec2() = default;
    Vec2(double x, double y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(double s) const { return {x * s, y * s}; }
    Vec2 operator/(double s) const { return {x / s, y / s}; }

    Vec2& operator+=(const Vec2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    double length() const { return std::sqrt(x * x + y * y); }
    Vec2 normalized() const {
        const double len = length();
        return len > 0.0 ? *this / len : Vec2{};
    }
};

inline Vec2 operator*(double s, const Vec2& v) { return v * s; }

}  // namespace renderer
