#pragma once

#include <filesystem>
#include <vector>

#include "raytracer/raytracer.hpp"

namespace cg::assets {

struct GltfTriangle {
    Vec3 first;
    Vec3 second;
    Vec3 third;
    Vec3 first_normal;
    Vec3 second_normal;
    Vec3 third_normal;
    bool has_normals{false};
    rt::Material material;
};

class GltfAsset {
public:
    [[nodiscard]] static GltfAsset load(const std::filesystem::path& path);
    [[nodiscard]] const std::vector<GltfTriangle>& triangles() const noexcept { return triangles_; }
    void add_to(rt::Scene& scene) const;

private:
    std::vector<GltfTriangle> triangles_;
};

}  // namespace cg::assets
