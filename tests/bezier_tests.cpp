#include <cmath>
#include <iostream>
#include <stdexcept>

#include "curves/bezier.hpp"

namespace {

bool near(double lhs, double rhs, double tolerance = 1e-8) { return std::abs(lhs - rhs) <= tolerance; }

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

}  // namespace

int main() {
    try {
        const cg::BezierCurve curve({{0.0, 0.0}, {1.0, 2.0}, {3.0, 2.0}, {4.0, 0.0}});
        require(near(curve.evaluate(0.0).x, 0.0), "curve should begin at its first control point");
        require(near(curve.evaluate(1.0).x, 4.0), "curve should end at its last control point");
        require(near(curve.derivative(0.0).x, 3.0) && near(curve.derivative(0.0).y, 6.0),
                "start derivative should match the first control edge");

        const cg::Vec2 midpoint = curve.evaluate(0.5);
        auto halves = curve.split(0.5);
        const cg::Vec2 left_end = halves.first.evaluate(1.0);
        const cg::Vec2 right_start = halves.second.evaluate(0.0);
        require(near(midpoint.x, left_end.x) && near(midpoint.y, left_end.y),
                "left split curve should end at the split point");
        require(near(midpoint.x, right_start.x) && near(midpoint.y, right_start.y),
                "right split curve should begin at the split point");
        require(curve.sample(16).size() == 17, "sampling should include both endpoints");

        std::cout << "Bezier tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Bezier tests failed: " << error.what() << '\n';
        return 1;
    }
}
