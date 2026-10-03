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

double fresnel_dielectric(double cosine, double eta_incident, double eta_transmitted) {
    cosine = clamp(std::abs(cosine));
    const double sine_transmitted = eta_incident / eta_transmitted *
                                    std::sqrt(std::max(0.0, 1.0 - cosine * cosine));
    if (sine_transmitted >= 1.0) return 1.0;
    const double cosine_transmitted = std::sqrt(std::max(0.0, 1.0 - sine_transmitted * sine_transmitted));
    const double parallel = (eta_transmitted * cosine - eta_incident * cosine_transmitted) /
                            (eta_transmitted * cosine + eta_incident * cosine_transmitted);
    const double perpendicular = (eta_incident * cosine - eta_transmitted * cosine_transmitted) /
                                 (eta_incident * cosine + eta_transmitted * cosine_transmitted);
    return 0.5 * (parallel * parallel + perpendicular * perpendicular);
}

bool refract(const Vec3& incident, const Vec3& normal, double eta_ratio, Vec3& transmitted) {
    const Vec3 direction = normalized(incident);
    const double cosine = std::min(1.0, -dot(direction, normal));
    const double discriminant = 1.0 - eta_ratio * eta_ratio * (1.0 - cosine * cosine);
    if (discriminant < 0.0) return false;
    transmitted = normalized(direction * eta_ratio + normal * (eta_ratio * cosine - std::sqrt(discriminant)));
    return true;
}

Vec3 sample_ggx_normal(const Vec3& normal, double roughness, double uniform_1, double uniform_2) {
    constexpr double tau = 2.0 * kPi;
    const double alpha = std::max(0.001, roughness * roughness);
    uniform_1 = clamp(uniform_1, 0.0, 1.0 - 1e-12);
    const double tangent_squared = alpha * alpha * uniform_1 / (1.0 - uniform_1);
    const double cosine = 1.0 / std::sqrt(1.0 + tangent_squared);
    const double sine = std::sqrt(std::max(0.0, 1.0 - cosine * cosine));
    const double azimuth = tau * uniform_2;
    const Vec3 helper = std::abs(normal.x) > 0.9 ? Vec3{0.0, 1.0, 0.0} : Vec3{1.0, 0.0, 0.0};
    const Vec3 tangent = normalized(cross(helper, normal));
    const Vec3 bitangent = cross(normal, tangent);
    return normalized(tangent * (sine * std::cos(azimuth)) +
                      bitangent * (sine * std::sin(azimuth)) + normal * cosine);
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
