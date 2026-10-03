#pragma once

#include "raytracer/raytracer.hpp"

namespace cg::sampling {

struct RectangleHit {
    double distance{0.0};
    double light_cosine{0.0};
};

[[nodiscard]] double power_heuristic(double first_pdf, double second_pdf);
[[nodiscard]] double cosine_hemisphere_pdf(double normal_cosine);
[[nodiscard]] bool intersect_area_light(const rt::AreaLight& light, const Vec3& origin,
                                        const Vec3& direction, RectangleHit& hit);
[[nodiscard]] double area_light_pdf(const rt::AreaLight& light, const Vec3& origin,
                                    const Vec3& direction);

}  // namespace cg::sampling
