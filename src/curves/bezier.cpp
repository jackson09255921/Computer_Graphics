#include "curves/bezier.hpp"

#include <stdexcept>

namespace cg {
namespace {

void validate_parameter(double t) {
    if (t < 0.0 || t > 1.0) {
        throw std::out_of_range("Bezier parameter must be in [0, 1]");
    }
}

Vec2 de_casteljau(std::vector<Vec2> points, double t) {
    for (std::size_t count = points.size(); count > 1; --count) {
        for (std::size_t index = 0; index + 1 < count; ++index) {
            points[index] = lerp(points[index], points[index + 1], t);
        }
    }
    return points.front();
}

}  // namespace

BezierCurve::BezierCurve(std::vector<Vec2> control_points) : control_points_(std::move(control_points)) {
    if (control_points_.size() < 2) {
        throw std::invalid_argument("a Bezier curve needs at least two control points");
    }
}

Vec2 BezierCurve::evaluate(double t) const {
    validate_parameter(t);
    return de_casteljau(control_points_, t);
}

Vec2 BezierCurve::derivative(double t) const {
    validate_parameter(t);
    std::vector<Vec2> derivative_points;
    derivative_points.reserve(degree());
    const double scale = static_cast<double>(degree());
    for (std::size_t index = 0; index < degree(); ++index) {
        derivative_points.push_back((control_points_[index + 1] - control_points_[index]) * scale);
    }
    return de_casteljau(std::move(derivative_points), t);
}

std::pair<BezierCurve, BezierCurve> BezierCurve::split(double t) const {
    validate_parameter(t);
    std::vector<Vec2> level = control_points_;
    std::vector<Vec2> left{level.front()};
    std::vector<Vec2> right{level.back()};
    while (level.size() > 1) {
        std::vector<Vec2> next;
        next.reserve(level.size() - 1);
        for (std::size_t index = 0; index + 1 < level.size(); ++index) {
            next.push_back(lerp(level[index], level[index + 1], t));
        }
        left.push_back(next.front());
        right.push_back(next.back());
        level = std::move(next);
    }
    return {BezierCurve{std::move(left)}, BezierCurve{std::vector<Vec2>(right.rbegin(), right.rend())}};
}

std::vector<Vec2> BezierCurve::sample(std::size_t segments) const {
    if (segments == 0) {
        throw std::invalid_argument("Bezier sampling needs at least one segment");
    }
    std::vector<Vec2> points;
    points.reserve(segments + 1);
    for (std::size_t index = 0; index <= segments; ++index) {
        points.push_back(evaluate(static_cast<double>(index) / static_cast<double>(segments)));
    }
    return points;
}

}  // namespace cg
