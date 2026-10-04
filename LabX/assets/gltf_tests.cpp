#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "assets/gltf_loader.hpp"

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(double lhs, double rhs, double tolerance = 1e-8) { return std::abs(lhs - rhs) < tolerance; }
template <typename T>
void write(std::ofstream& output, T value) {
    output.write(reinterpret_cast<const char*>(&value), sizeof(value));
}
void append_u32(std::vector<std::uint8_t>& output, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        output.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffu));
}
}

int main() {
    const std::filesystem::path gltf_path = "gltf_test.gltf";
    const std::filesystem::path bin_path = "gltf_test.bin";
    const std::filesystem::path glb_path = "gltf_test.glb";
    try {
        {
            std::ofstream output(bin_path, std::ios::binary);
            const float positions[] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
            for (float value : positions) write(output, value);
            write<std::uint16_t>(output, 0);
            write<std::uint16_t>(output, 1);
            write<std::uint16_t>(output, 2);
            const float normals[] = {0, 0, 1, 0, 1, 0, 1, 0, 0};
            for (float value : normals) write(output, value);
        }
        {
            std::ofstream output(gltf_path);
            output << R"({
  "asset":{"version":"2.0"},
  "buffers":[{"uri":"gltf_test.bin","byteLength":78}],
  "bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":6},
    {"buffer":0,"byteOffset":42,"byteLength":36}],
  "accessors":[
    {"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"},
    {"bufferView":1,"componentType":5123,"count":3,"type":"SCALAR"},
    {"bufferView":2,"componentType":5126,"count":3,"type":"VEC3"}
  ],
  "materials":[{"doubleSided":true,"alphaMode":"MASK","alphaCutoff":0.35,"emissiveFactor":[0.1,0.2,0.3],"extensions":{"KHR_materials_transmission":{"transmissionFactor":0.7},"KHR_materials_ior":{"ior":1.4},"KHR_materials_clearcoat":{"clearcoatFactor":0.8,"clearcoatRoughnessFactor":0.12},"KHR_materials_sheen":{"sheenColorFactor":[0.9,0.2,0.1],"sheenRoughnessFactor":0.45}},"pbrMetallicRoughness":{"baseColorFactor":[0.2,0.4,0.8,0.75],"metallicFactor":0.6,"roughnessFactor":0.25}}],
  "meshes":[{"primitives":[{"attributes":{"POSITION":0,"NORMAL":2},"indices":1,"material":0}]}],
  "nodes":[{"mesh":0,"translation":[2,3,4],"scale":[2,2,2]}],
  "scenes":[{"nodes":[0]}],"scene":0
})";
        }

        const cg::assets::GltfAsset asset = cg::assets::GltfAsset::load(gltf_path);
        require(asset.triangles().size() == 1, "one indexed glTF triangle must be loaded");
        const cg::assets::GltfTriangle& triangle = asset.triangles().front();
        require(near(triangle.first.x, 2.0) && near(triangle.first.y, 3.0) && near(triangle.first.z, 4.0),
                "node translation must transform glTF positions");
        require(near(triangle.second.x, 4.0) && near(triangle.third.y, 5.0),
                "node scale must transform glTF positions");
        require(near(triangle.material.albedo.z, 0.8) && near(triangle.material.metallic, 0.6) &&
                    near(triangle.material.roughness, 0.25),
                "metallic-roughness material factors must be imported");
        require(near(triangle.emissive_factor.x, 0.1) && near(triangle.emissive_factor.y, 0.2) &&
                    near(triangle.emissive_factor.z, 0.3),
                "emissive material factor must be imported");
        require(near(triangle.material.transmission, 0.7) &&
                    near(triangle.material.index_of_refraction, 1.4),
                "transmission and IOR extensions must be imported");
        require(triangle.transmission_texture == nullptr,
                "transmission texture slot must retain a safe default when omitted");
        require(near(triangle.specular_factor, 1.0) &&
                    near(triangle.specular_color_factor.x, 1.0) &&
                    triangle.specular_texture == nullptr &&
                    triangle.specular_color_texture == nullptr,
                "specular extension inputs must retain glTF defaults when omitted");
        require(near(triangle.anisotropy_strength, 0.0) &&
                    near(triangle.anisotropy_rotation, 0.0) &&
                    triangle.anisotropy_texture == nullptr,
                "anisotropy extension inputs must retain glTF defaults when omitted");
        require(near(triangle.clearcoat_factor, 0.8) && near(triangle.clearcoat_roughness, 0.12),
                "clearcoat extension factors must be imported");
        require(triangle.clearcoat_texture == nullptr &&
                    triangle.clearcoat_roughness_texture == nullptr &&
                    triangle.clearcoat_normal_texture == nullptr &&
                    near(triangle.clearcoat_normal_scale, 1.0),
                "clearcoat texture slots must retain safe defaults when omitted");
        require(near(triangle.sheen_color_factor.x, 0.9) &&
                    near(triangle.sheen_color_factor.y, 0.2) &&
                    near(triangle.sheen_color_factor.z, 0.1) && near(triangle.sheen_roughness, 0.45),
                "sheen extension factors must be imported");
        require(triangle.sheen_color_texture == nullptr &&
                    triangle.sheen_roughness_texture == nullptr,
                "sheen texture slots must retain safe defaults when omitted");
        require(triangle.occlusion_texture == nullptr && near(triangle.occlusion_strength, 1.0),
                "missing occlusion texture must retain the neutral defaults");
        require(triangle.double_sided, "double-sided material state must be imported");
        require(triangle.alpha_mode == 1 && near(triangle.alpha_cutoff, 0.35) &&
                    near(triangle.base_color_alpha, 0.75),
                "alpha mode, cutoff, and base-color alpha must be imported");
        require(triangle.material_index == 0, "primitive material index must be retained for scene framing");
        require(triangle.has_normals && near(triangle.first_normal.z, 1.0) &&
                    near(triangle.second_normal.y, 1.0) && near(triangle.third_normal.x, 1.0),
                "NORMAL accessor must be imported and transformed");

        cg::rt::Scene scene;
        asset.add_to(scene);
        scene.build();
        cg::rt::Hit hit;
        require(scene.intersect({{2.5, 3.5, 6.0}, {0.0, 0.0, -1.0}}, 1e-5, 10.0, hit),
                "imported glTF triangles must be usable by the ray tracer");
        require(near(hit.normal.x, 0.408248290463863, 1e-6) &&
                    near(hit.normal.y, 0.408248290463863, 1e-6) &&
                    near(hit.normal.z, 0.816496580927726, 1e-6),
                "ray-triangle hits must barycentrically interpolate vertex normals");

        {
            std::ofstream output(gltf_path, std::ios::trunc);
            output << R"({"asset":{"version":"2.0"},"buffers":[{"uri":"gltf_test.bin","byteLength":78}],
"bufferViews":[{"buffer":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":6},
{"buffer":0,"byteOffset":42,"byteLength":36}],
"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"},
{"bufferView":1,"componentType":5123,"count":3,"type":"SCALAR"},
{"bufferView":2,"componentType":5126,"count":3,"type":"VEC3"}],
"meshes":[{"primitives":[{"attributes":{"POSITION":0,"NORMAL":2},"indices":1}]}],
"nodes":[{"mesh":0,"scale":[1,1,0]}]})";
        }
        const cg::assets::GltfAsset singular_asset = cg::assets::GltfAsset::load(gltf_path);
        require(singular_asset.triangles().size() == 1 &&
                    near(singular_asset.triangles()[0].first_normal.z, 1.0),
                "singular node transforms must fall back to the geometric normal");

        {
            std::ofstream output(gltf_path, std::ios::trunc);
            output << R"({"asset":{"version":"2.0"},"buffers":[{"uri":"gltf_test.bin","byteLength":42}],
"bufferViews":[{"buffer":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":6}],
"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"},
{"bufferView":1,"componentType":5123,"count":3,"type":"SCALAR"}],
"meshes":[{"primitives":[{"attributes":{"POSITION":0},"indices":1}]}],"nodes":[{"mesh":0}]})";
        }
        const cg::assets::GltfAsset default_scene_asset = cg::assets::GltfAsset::load(gltf_path);
        require(default_scene_asset.triangles().size() == 1 &&
                    near(default_scene_asset.triangles()[0].material.albedo.x, 0.8),
                "root nodes and the default material must work without scenes or materials arrays");

        const std::string glb_json = R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":42}],
"bufferViews":[{"buffer":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":6}],
"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"},
{"bufferView":1,"componentType":5123,"count":3,"type":"SCALAR"}],
"meshes":[{"primitives":[{"attributes":{"POSITION":0},"indices":1}]}],
"nodes":[{"mesh":0}],"scenes":[{"nodes":[0]}],"scene":0})";
        std::vector<std::uint8_t> json_chunk(glb_json.begin(), glb_json.end());
        while (json_chunk.size() % 4 != 0) json_chunk.push_back(' ');
        std::ifstream binary_input(bin_path, std::ios::binary);
        const std::vector<std::uint8_t> bin_chunk{std::istreambuf_iterator<char>(binary_input),
                                                  std::istreambuf_iterator<char>()};
        std::vector<std::uint8_t> glb;
        glb.insert(glb.end(), {'g', 'l', 'T', 'F'});
        append_u32(glb, 2);
        append_u32(glb, static_cast<std::uint32_t>(12 + 8 + json_chunk.size() + 8 + bin_chunk.size() + 2));
        append_u32(glb, static_cast<std::uint32_t>(json_chunk.size()));
        append_u32(glb, 0x4e4f534au);
        glb.insert(glb.end(), json_chunk.begin(), json_chunk.end());
        append_u32(glb, static_cast<std::uint32_t>(bin_chunk.size() + 2));
        append_u32(glb, 0x004e4942u);
        glb.insert(glb.end(), bin_chunk.begin(), bin_chunk.end());
        glb.insert(glb.end(), {0, 0});
        {
            std::ofstream output(glb_path, std::ios::binary);
            output.write(reinterpret_cast<const char*>(glb.data()), static_cast<std::streamsize>(glb.size()));
        }
        const cg::assets::GltfAsset glb_asset = cg::assets::GltfAsset::load(glb_path);
        require(glb_asset.triangles().size() == 1, "GLB v2 JSON and BIN chunks must load");

        const cg::assets::GltfTexture texture{2, 2,
            {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 1, 1}}, {0.25, 0.5, 0.75, 1.0}};
        const cg::Color first_texel = texture.sample({0.25, 0.25});
        const cg::Color repeated_texel = texture.sample({1.25, 0.25});
        require(near(first_texel.x, 1.0) && near(first_texel.y, 0.0) &&
                    near(repeated_texel.x, first_texel.x),
                "base-color texture sampling must be bilinear and repeat UV coordinates");
        require(near(texture.sample_alpha({0.25, 0.25}), 0.25) &&
                    near(texture.sample_alpha({1.25, 0.25}), 0.25),
                "alpha texture sampling must be bilinear and repeat UV coordinates");
        cg::assets::GltfTexture clamped = texture;
        clamped.wrap_s = 33071;
        require(near(clamped.sample({1.25, 0.25}).y, 1.0),
                "clamp-to-edge sampler mode must retain the last column");
        cg::assets::GltfTexture mirrored = texture;
        mirrored.wrap_s = 33648;
        require(near(mirrored.sample({1.25, 0.25}).y, 1.0),
                "mirrored-repeat sampler mode must reflect alternate periods");

        std::filesystem::remove(gltf_path);
        std::filesystem::remove(bin_path);
        std::filesystem::remove(glb_path);
        std::cout << "glTF tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::filesystem::remove(gltf_path);
        std::filesystem::remove(bin_path);
        std::filesystem::remove(glb_path);
        std::cerr << "glTF tests failed: " << error.what() << '\n';
        return 1;
    }
}
