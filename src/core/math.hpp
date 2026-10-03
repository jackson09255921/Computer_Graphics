#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace cg {

constexpr double kEpsilon = 1e-9;

struct Vec2 {
    double x{0.0};
    double y{0.0};

    constexpr Vec2 operator+(const Vec2& rhs) const { return {x + rhs.x, y + rhs.y}; }
    constexpr Vec2 operator-(const Vec2& rhs) const { return {x - rhs.x, y - rhs.y}; }
    constexpr Vec2 operator*(double scalar) const { return {x * scalar, y * scalar}; }
    constexpr Vec2 operator/(double scalar) const { return {x / scalar, y / scalar}; }
};

constexpr Vec2 operator*(double scalar, const Vec2& value) { return value * scalar; }
constexpr double dot(const Vec2& lhs, const Vec2& rhs) { return lhs.x * rhs.x + lhs.y * rhs.y; }
inline double length(const Vec2& value) { return std::sqrt(dot(value, value)); }
inline Vec2 normalized(const Vec2& value) {
    const double magnitude = length(value);
    return magnitude <= kEpsilon ? Vec2{} : value / magnitude;
}

struct Vec3 {
    double x{0.0};
    double y{0.0};
    double z{0.0};

    constexpr Vec3 operator+() const { return *this; }
    constexpr Vec3 operator-() const { return {-x, -y, -z}; }
    constexpr Vec3 operator+(const Vec3& rhs) const { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
    constexpr Vec3 operator-(const Vec3& rhs) const { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
    constexpr Vec3 operator*(double scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    constexpr Vec3 operator/(double scalar) const { return {x / scalar, y / scalar, z / scalar}; }
    constexpr Vec3 operator*(const Vec3& rhs) const { return {x * rhs.x, y * rhs.y, z * rhs.z}; }

    Vec3& operator+=(const Vec3& rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }
};

constexpr Vec3 operator*(double scalar, const Vec3& value) { return value * scalar; }
constexpr double dot(const Vec3& lhs, const Vec3& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}
constexpr Vec3 cross(const Vec3& lhs, const Vec3& rhs) {
    return {
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x,
    };
}
inline double length(const Vec3& value) { return std::sqrt(dot(value, value)); }
inline Vec3 normalized(const Vec3& value) {
    const double magnitude = length(value);
    return magnitude <= kEpsilon ? Vec3{} : value / magnitude;
}

template <typename T>
constexpr T lerp(const T& start, const T& end, double t) {
    return start * (1.0 - t) + end * t;
}

inline double clamp(double value, double minimum = 0.0, double maximum = 1.0) {
    return std::clamp(value, minimum, maximum);
}

struct Quaternion {
    double w{1.0};
    double x{0.0};
    double y{0.0};
    double z{0.0};

    constexpr Quaternion operator+(const Quaternion& rhs) const {
        return {w + rhs.w, x + rhs.x, y + rhs.y, z + rhs.z};
    }
    constexpr Quaternion operator-() const { return {-w, -x, -y, -z}; }
    constexpr Quaternion operator*(double scalar) const {
        return {w * scalar, x * scalar, y * scalar, z * scalar};
    }
    constexpr Quaternion operator*(const Quaternion& rhs) const {
        return {
            w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z,
            w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
            w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
            w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w,
        };
    }
};

constexpr double dot(const Quaternion& lhs, const Quaternion& rhs) {
    return lhs.w * rhs.w + lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}
inline Quaternion normalized(const Quaternion& value) {
    const double magnitude = std::sqrt(dot(value, value));
    return magnitude <= kEpsilon ? Quaternion{} : value * (1.0 / magnitude);
}
inline Quaternion from_axis_angle(const Vec3& axis, double radians) {
    const Vec3 unit_axis = normalized(axis);
    const double half_angle = radians * 0.5;
    const double sine = std::sin(half_angle);
    return normalized({std::cos(half_angle), unit_axis.x * sine, unit_axis.y * sine, unit_axis.z * sine});
}
inline Quaternion slerp(Quaternion start, Quaternion end, double t) {
    start = normalized(start);
    end = normalized(end);
    double cosine = dot(start, end);
    if (cosine < 0.0) {
        end = -end;
        cosine = -cosine;
    }
    if (cosine > 0.9995) {
        return normalized(start * (1.0 - t) + end * t);
    }
    const double angle = std::acos(clamp(cosine, -1.0, 1.0));
    const double sine = std::sin(angle);
    return normalized(start * (std::sin((1.0 - t) * angle) / sine) + end * (std::sin(t * angle) / sine));
}
inline Vec3 rotate(const Quaternion& rotation, const Vec3& value) {
    const Quaternion unit = normalized(rotation);
    const Quaternion vector{0.0, value.x, value.y, value.z};
    const Quaternion conjugate{unit.w, -unit.x, -unit.y, -unit.z};
    const Quaternion result = unit * vector * conjugate;
    return {result.x, result.y, result.z};
}

}  // namespace cg
