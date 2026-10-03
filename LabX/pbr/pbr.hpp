#pragma once

#include "core/image.hpp"

namespace cg::pbr {

[[nodiscard]] Color fresnel_schlick(double cosine, const Color& reflectance_at_normal);
[[nodiscard]] double ggx_distribution(double normal_dot_half, double roughness);
[[nodiscard]] double smith_geometry(double normal_dot_view, double normal_dot_light, double roughness);
[[nodiscard]] Color evaluate_ggx(const Vec3& normal, const Vec3& view, const Vec3& light,
                                 const Color& base_color, double metallic, double roughness);

}  // namespace cg::pbr
