#pragma once

#include <vector>

#include "core/math.hpp"

namespace cg::animation {

struct Transform {
    Vec3 position{};
    Quaternion rotation{};
    Vec3 scale{1.0, 1.0, 1.0};
};

struct Keyframe {
    double time{0.0};
    Transform transform{};
};

class CubicBezierEasing {
public:
    CubicBezierEasing(double x1, double y1, double x2, double y2);
    [[nodiscard]] double evaluate(double progress) const;

private:
    [[nodiscard]] double sample_x(double parameter) const;
    [[nodiscard]] double sample_y(double parameter) const;
    [[nodiscard]] double sample_x_derivative(double parameter) const;

    double x1_;
    double y1_;
    double x2_;
    double y2_;
};

class AnimationClip {
public:
    AnimationClip(std::vector<Keyframe> keyframes, CubicBezierEasing easing = {0.42, 0.0, 0.58, 1.0});

    [[nodiscard]] double duration() const noexcept { return keyframes_.back().time; }
    [[nodiscard]] Transform sample(double time) const;

private:
    std::vector<Keyframe> keyframes_;
    CubicBezierEasing easing_;
};

[[nodiscard]] Vec3 transform_point(const Transform& transform, const Vec3& point);

}  // namespace cg::animation
