#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "core/image.hpp"
#include "core/math.hpp"

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
        const cg::Vec3 x_axis{1.0, 0.0, 0.0};
        const cg::Vec3 y_axis{0.0, 1.0, 0.0};
        const cg::Vec3 z_axis = cg::cross(x_axis, y_axis);
        require(near(z_axis.z, 1.0), "cross product should produce the Z axis");
        require(near(cg::dot(x_axis, y_axis), 0.0), "orthogonal vectors should have zero dot product");

        constexpr double pi = 3.14159265358979323846;
        const cg::Quaternion rotation = cg::from_axis_angle(z_axis, pi * 0.5);
        const cg::Vec3 rotated = cg::rotate(rotation, x_axis);
        require(near(rotated.x, 0.0) && near(rotated.y, 1.0), "quaternion rotation should rotate X to Y");

        const cg::Quaternion halfway = cg::slerp(cg::Quaternion{}, rotation, 0.5);
        const cg::Vec3 diagonal = cg::rotate(halfway, x_axis);
        require(near(diagonal.x, std::sqrt(0.5)) && near(diagonal.y, std::sqrt(0.5)),
                "SLERP should produce a 45 degree rotation");

        cg::Image image(2, 2);
        image.set(1, 0, {1.0, 0.0, 0.0});
        require(near(image.get(1, 0).x, 1.0), "image should retain pixel colors");

        std::cout << "core tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "core tests failed: " << error.what() << '\n';
        return 1;
    }
}
