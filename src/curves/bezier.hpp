#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "core/math.hpp"

namespace cg {

class BezierCurve {
public:
    explicit BezierCurve(std::vector<Vec2> control_points);

    [[nodiscard]] const std::vector<Vec2>& control_points() const noexcept { return control_points_; }
    [[nodiscard]] std::size_t degree() const noexcept { return control_points_.size() - 1; }
    [[nodiscard]] Vec2 evaluate(double t) const;
    [[nodiscard]] Vec2 derivative(double t) const;
    [[nodiscard]] std::pair<BezierCurve, BezierCurve> split(double t) const;
    [[nodiscard]] std::vector<Vec2> sample(std::size_t segments) const;

private:
    std::vector<Vec2> control_points_;
};

}  // namespace cg
