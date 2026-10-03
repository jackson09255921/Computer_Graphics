#include <cmath>
#include <iostream>
#include <stdexcept>

#include "animation/animation.hpp"

namespace {

bool near(double lhs, double rhs, double tolerance = 1e-7) { return std::abs(lhs - rhs) <= tolerance; }

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

}  // namespace

int main() {
    try {
        const cg::animation::CubicBezierEasing linear(0.0, 0.0, 1.0, 1.0);
        require(near(linear.evaluate(0.25), 0.25), "linear Bezier easing should preserve progress");

        constexpr double pi = 3.14159265358979323846;
        const cg::animation::AnimationClip clip({
            {0.0, {{0.0, 0.0, 0.0}, {}, {1.0, 1.0, 1.0}}},
            {2.0, {{2.0, 4.0, 0.0}, cg::from_axis_angle({0.0, 0.0, 1.0}, pi), {3.0, 3.0, 3.0}}},
        }, linear);
        const cg::animation::Transform midpoint = clip.sample(1.0);
        require(near(midpoint.position.x, 1.0) && near(midpoint.position.y, 2.0),
                "translation should interpolate at the midpoint");
        require(near(midpoint.scale.x, 2.0), "scale should interpolate at the midpoint");
        const cg::Vec3 rotated = cg::rotate(midpoint.rotation, {1.0, 0.0, 0.0});
        require(near(rotated.x, 0.0) && near(rotated.y, 1.0), "SLERP should produce a 90 degree midpoint");

        std::cout << "animation tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "animation tests failed: " << error.what() << '\n';
        return 1;
    }
}
