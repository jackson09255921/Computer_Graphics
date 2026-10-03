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
        require(std::abs(cg::pbr::fresnel_dielectric(1.0, 1.0, 1.5) - 0.04) < 1e-9,
                "air-to-glass normal-incidence Fresnel must be four percent");
        cg::Vec3 transmitted;
        require(cg::pbr::refract({0.0, -1.0, 0.0}, {0.0, 1.0, 0.0}, 1.0 / 1.5, transmitted),
                "normal-incidence refraction must succeed");
        require(std::abs(transmitted.y + 1.0) < 1e-9, "normal-incidence refraction must remain straight");
        require(!cg::pbr::refract({0.8660254038, -0.5, 0.0}, {0.0, 1.0, 0.0}, 1.5, transmitted),
                "glass-to-air ray above the critical angle must totally reflect");
        const cg::Vec3 microfacet = cg::pbr::sample_ggx_normal({0.0, 1.0, 0.0}, 0.35, 0.4, 0.7);
        require(std::abs(cg::length(microfacet) - 1.0) < 1e-9 && microfacet.y > 0.0,
                "GGX microfacet normals must be unit vectors above the surface");
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
