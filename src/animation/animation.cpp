#include "animation/animation.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace cg::animation {

CubicBezierEasing::CubicBezierEasing(double x1, double y1, double x2, double y2)
    : x1_(x1), y1_(y1), x2_(x2), y2_(y2) {
    if (x1 < 0.0 || x1 > 1.0 || x2 < 0.0 || x2 > 1.0) {
        throw std::invalid_argument("Bezier easing X control points must be in [0, 1]");
    }
}

double CubicBezierEasing::sample_x(double parameter) const {
    const double inverse = 1.0 - parameter;
    return 3.0 * inverse * inverse * parameter * x1_ + 3.0 * inverse * parameter * parameter * x2_ +
           parameter * parameter * parameter;
}

double CubicBezierEasing::sample_y(double parameter) const {
    const double inverse = 1.0 - parameter;
    return 3.0 * inverse * inverse * parameter * y1_ + 3.0 * inverse * parameter * parameter * y2_ +
           parameter * parameter * parameter;
}

double CubicBezierEasing::sample_x_derivative(double parameter) const {
    const double inverse = 1.0 - parameter;
    return 3.0 * inverse * inverse * x1_ + 6.0 * inverse * parameter * (x2_ - x1_) +
           3.0 * parameter * parameter * (1.0 - x2_);
}

double CubicBezierEasing::evaluate(double progress) const {
    progress = clamp(progress);
    double parameter = progress;
    for (int iteration = 0; iteration < 8; ++iteration) {
        const double error = sample_x(parameter) - progress;
        const double derivative = sample_x_derivative(parameter);
        if (std::abs(error) <= 1e-7 || std::abs(derivative) <= kEpsilon) {
            break;
        }
        parameter = clamp(parameter - error / derivative);
    }

    double lower = 0.0;
    double upper = 1.0;
    for (int iteration = 0; iteration < 12 && std::abs(sample_x(parameter) - progress) > 1e-7; ++iteration) {
        if (sample_x(parameter) < progress) {
            lower = parameter;
        } else {
            upper = parameter;
        }
        parameter = (lower + upper) * 0.5;
    }
    return sample_y(parameter);
}

AnimationClip::AnimationClip(std::vector<Keyframe> keyframes, CubicBezierEasing easing)
    : keyframes_(std::move(keyframes)), easing_(easing) {
    if (keyframes_.size() < 2) {
        throw std::invalid_argument("an animation clip needs at least two keyframes");
    }
    if (keyframes_.front().time < 0.0) {
        throw std::invalid_argument("keyframe time must not be negative");
    }
    for (std::size_t index = 1; index < keyframes_.size(); ++index) {
        if (keyframes_[index].time <= keyframes_[index - 1].time) {
            throw std::invalid_argument("keyframe times must be strictly increasing");
        }
    }
}

Transform AnimationClip::sample(double time) const {
    if (time <= keyframes_.front().time) {
        return keyframes_.front().transform;
    }
    if (time >= keyframes_.back().time) {
        return keyframes_.back().transform;
    }

    const auto upper = std::upper_bound(keyframes_.begin(), keyframes_.end(), time,
                                        [](double value, const Keyframe& frame) { return value < frame.time; });
    const Keyframe& end = *upper;
    const Keyframe& start = *(upper - 1);
    const double local = (time - start.time) / (end.time - start.time);
    const double eased = easing_.evaluate(local);
    return {
        lerp(start.transform.position, end.transform.position, eased),
        slerp(start.transform.rotation, end.transform.rotation, eased),
        lerp(start.transform.scale, end.transform.scale, eased),
    };
}

Vec3 transform_point(const Transform& transform, const Vec3& point) {
    return rotate(transform.rotation, point * transform.scale) + transform.position;
}

}  // namespace cg::animation
