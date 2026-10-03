#include <cmath>
#include <iostream>
#include <stdexcept>

#include "pbr/pbr.hpp"

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
bool finite(const cg::Color& color) {
    return std::isfinite(color.x) && std::isfinite(color.y) && std::isfinite(color.z);
}
}  // namespace

int main() {
    try {
        const cg::Color f0{0.04, 0.04, 0.04};
        const cg::Color head_on = cg::pbr::fresnel_schlick(1.0, f0);
        const cg::Color grazing = cg::pbr::fresnel_schlick(0.0, f0);
        require(std::abs(head_on.x - 0.04) < 1e-9, "normal-incidence Fresnel must equal F0");
        require(std::abs(grazing.x - 1.0) < 1e-9, "grazing Fresnel must approach one");
        require(cg::pbr::ggx_distribution(1.0, 0.1) > cg::pbr::ggx_distribution(1.0, 0.8),
                "smooth surfaces must have a sharper GGX peak");
        const cg::Color brdf = cg::pbr::evaluate_ggx({0.0, 1.0, 0.0}, {0.0, 1.0, 0.0},
                                                     {0.0, 1.0, 0.0}, {0.8, 0.2, 0.1}, 0.5, 0.4);
        require(finite(brdf) && brdf.x > 0.0, "GGX BRDF must produce finite positive energy");
        std::cout << "PBR tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "PBR tests failed: " << error.what() << '\n';
        return 1;
    }
}
