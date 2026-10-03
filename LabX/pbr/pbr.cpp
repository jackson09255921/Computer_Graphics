#include "pbr/pbr.hpp"

#include <algorithm>
#include <cmath>

namespace cg::pbr {
namespace {

constexpr double kPi = 3.14159265358979323846;

double smith_schlick(double normal_dot_direction, double roughness) {
    const double r = roughness + 1.0;
    const double k = r * r / 8.0;
    return normal_dot_direction / (normal_dot_direction * (1.0 - k) + k);
}

}  // namespace

Color fresnel_schlick(double cosine, const Color& reflectance_at_normal) {
    const double factor = std::pow(1.0 - clamp(cosine), 5.0);
    return reflectance_at_normal + (Color{1.0, 1.0, 1.0} - reflectance_at_normal) * factor;
}

double ggx_distribution(double normal_dot_half, double roughness) {
    const double alpha = std::max(0.045, roughness * roughness);
    const double alpha_squared = alpha * alpha;
    const double cosine_squared = clamp(normal_dot_half) * clamp(normal_dot_half);
    const double denominator = cosine_squared * (alpha_squared - 1.0) + 1.0;
    return alpha_squared / (kPi * denominator * denominator);
}

double smith_geometry(double normal_dot_view, double normal_dot_light, double roughness) {
    return smith_schlick(clamp(normal_dot_view), roughness) *
           smith_schlick(clamp(normal_dot_light), roughness);
}

Color evaluate_ggx(const Vec3& normal, const Vec3& view, const Vec3& light,
                   const Color& base_color, double metallic, double roughness) {
    const double n_dot_v = std::max(0.0, dot(normal, view));
    const double n_dot_l = std::max(0.0, dot(normal, light));
    if (n_dot_v <= 0.0 || n_dot_l <= 0.0) {
        return {};
    }
    const Vec3 half_vector = normalized(view + light);
    const double n_dot_h = std::max(0.0, dot(normal, half_vector));
    const double v_dot_h = std::max(0.0, dot(view, half_vector));
    metallic = clamp(metallic);
    roughness = clamp(roughness, 0.045, 1.0);
    const Color f0 = lerp(Color{0.04, 0.04, 0.04}, base_color, metallic);
    const Color fresnel = fresnel_schlick(v_dot_h, f0);
    const double distribution = ggx_distribution(n_dot_h, roughness);
    const double geometry = smith_geometry(n_dot_v, n_dot_l, roughness);
    const Color specular = fresnel * (distribution * geometry / std::max(4.0 * n_dot_v * n_dot_l, 1e-8));
    const Color diffuse = base_color * (1.0 - metallic) * (Color{1.0, 1.0, 1.0} - fresnel) / kPi;
    return diffuse + specular;
}

}  // namespace cg::pbr
