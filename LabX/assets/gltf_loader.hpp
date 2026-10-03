#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include "raytracer/raytracer.hpp"

namespace cg::assets {

struct GltfTexture {
    std::size_t width{};
    std::size_t height{};
    std::vector<Color> pixels;
    [[nodiscard]] Color sample(Vec2 uv) const;
};

struct GltfTriangle {
    Vec3 first;
    Vec3 second;
    Vec3 third;
    Vec3 first_normal;
    Vec3 second_normal;
    Vec3 third_normal;
    bool has_normals{false};
    Vec2 first_uv;
    Vec2 second_uv;
    Vec2 third_uv;
    std::shared_ptr<const GltfTexture> base_color_texture;
    std::shared_ptr<const GltfTexture> normal_texture;
    double normal_scale{1.0};
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
