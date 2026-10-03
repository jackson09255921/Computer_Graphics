#include <cmath>
#include <iostream>
#include <stdexcept>

#include "sampling/mis.hpp"

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}  // namespace

int main() {
    try {
        require(std::abs(cg::sampling::power_heuristic(1.0, 1.0) - 0.5) < 1e-12,
                "equal PDFs must receive equal MIS weight");
        require(cg::sampling::power_heuristic(4.0, 1.0) > 0.94,
                "the stronger sampling technique must dominate");
        require(std::abs(cg::sampling::cosine_hemisphere_pdf(1.0) - 1.0 / 3.14159265358979323846) < 1e-12,
                "cosine hemisphere PDF is incorrect");

        const cg::rt::AreaLight light{{0.0, 2.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0},
                                       {1.0, 1.0, 1.0}, 1.0};
        cg::sampling::RectangleHit hit;
        require(cg::sampling::intersect_area_light(light, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, hit),
                "center ray must hit the rectangle light");
        require(std::abs(hit.distance - 2.0) < 1e-12, "rectangle distance is incorrect");
        require(std::abs(cg::sampling::area_light_pdf(light, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}) - 1.0) < 1e-12,
                "solid-angle area-light PDF is incorrect");
        require(!cg::sampling::intersect_area_light(light, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, hit),
                "parallel ray must miss the rectangle light");
        std::cout << "MIS tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "MIS tests failed: " << error.what() << '\n';
        return 1;
    }
}
