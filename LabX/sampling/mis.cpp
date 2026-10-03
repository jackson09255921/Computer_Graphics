#include "sampling/mis.hpp"

#include <algorithm>
#include <cmath>

namespace cg::sampling {

double power_heuristic(double first_pdf, double second_pdf) {
    const double first_squared = first_pdf * first_pdf;
    const double second_squared = second_pdf * second_pdf;
    const double sum = first_squared + second_squared;
    return sum <= kEpsilon ? 0.0 : first_squared / sum;
}

double cosine_hemisphere_pdf(double normal_cosine) {
    constexpr double inverse_pi = 0.31830988618379067154;
    return std::max(0.0, normal_cosine) * inverse_pi;
}

bool intersect_area_light(const rt::AreaLight& light, const Vec3& origin,
                          const Vec3& direction, RectangleHit& hit) {
    const Vec3 normal = light.normal();
    const double denominator = dot(normal, direction);
    if (std::abs(denominator) <= kEpsilon) return false;
    const double distance = dot(light.center - origin, normal) / denominator;
    if (distance <= kEpsilon) return false;
    const Vec3 local = origin + direction * distance - light.center;
    const double u_length_squared = dot(light.half_u, light.half_u);
    const double v_length_squared = dot(light.half_v, light.half_v);
    if (u_length_squared <= kEpsilon || v_length_squared <= kEpsilon) return false;
    const double u = dot(local, light.half_u) / u_length_squared;
    const double v = dot(local, light.half_v) / v_length_squared;
    if (std::abs(u) > 1.0 || std::abs(v) > 1.0) return false;
    const double light_cosine = std::max(0.0, dot(normal, -direction));
    if (light_cosine <= 0.0) return false;
    hit = {distance, light_cosine};
    return true;
}

double area_light_pdf(const rt::AreaLight& light, const Vec3& origin, const Vec3& direction) {
    RectangleHit hit;
    if (!intersect_area_light(light, origin, direction, hit)) return 0.0;
    return hit.distance * hit.distance / (hit.light_cosine * light.area());
}

}  // namespace cg::sampling
