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
    std::vector<double> alpha;
    int wrap_s{10497};
    int wrap_t{10497};
    [[nodiscard]] Color sample(Vec2 uv) const;
    [[nodiscard]] double sample_alpha(Vec2 uv) const;
};

struct GltfTextureMapping {
    int texcoord{0};
    Vec2 offset{};
    Vec2 scale{1.0, 1.0};
    double rotation{0.0};
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
    std::shared_ptr<const GltfTexture> metallic_roughness_texture;
    std::shared_ptr<const GltfTexture> emissive_texture;
    std::shared_ptr<const GltfTexture> clearcoat_texture;
    std::shared_ptr<const GltfTexture> clearcoat_roughness_texture;
    std::shared_ptr<const GltfTexture> clearcoat_normal_texture;
    std::shared_ptr<const GltfTexture> sheen_color_texture;
    std::shared_ptr<const GltfTexture> sheen_roughness_texture;
    std::shared_ptr<const GltfTexture> occlusion_texture;
    Color emissive_factor{};
    Color sheen_color_factor{};
    double sheen_roughness{0.0};
    double clearcoat_factor{0.0};
    double clearcoat_roughness{0.0};
    double clearcoat_normal_scale{1.0};
    double normal_scale{1.0};
    rt::Material material;
    int material_index{-1};
    double occlusion_strength{1.0};
    bool double_sided{false};
    int alpha_mode{0};  // 0: OPAQUE, 1: MASK, 2: BLEND
    double alpha_cutoff{0.5};
    double base_color_alpha{1.0};
    Vec2 first_uv1;
    Vec2 second_uv1;
    Vec2 third_uv1;
    GltfTextureMapping base_color_mapping;
    GltfTextureMapping normal_mapping;
    GltfTextureMapping metallic_roughness_mapping;
    GltfTextureMapping emissive_mapping;
    GltfTextureMapping clearcoat_mapping;
    GltfTextureMapping clearcoat_roughness_mapping;
    GltfTextureMapping clearcoat_normal_mapping;
    GltfTextureMapping sheen_color_mapping;
    GltfTextureMapping sheen_roughness_mapping;
    GltfTextureMapping occlusion_mapping;
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
