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
bool near(double lhs, double rhs) { return std::abs(lhs - rhs) < 1e-8; }
template <typename T>
void write(std::ofstream& output, T value) {
    output.write(reinterpret_cast<const char*>(&value), sizeof(value));
}
}

int main() {
    const std::filesystem::path gltf_path = "gltf_test.gltf";
    const std::filesystem::path bin_path = "gltf_test.bin";
    try {
        {
            std::ofstream output(bin_path, std::ios::binary);
            const float positions[] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
            for (float value : positions) write(output, value);
            write<std::uint16_t>(output, 0);
            write<std::uint16_t>(output, 1);
            write<std::uint16_t>(output, 2);
        }
        {
            std::ofstream output(gltf_path);
            output << R"({
  "asset":{"version":"2.0"},
  "buffers":[{"uri":"gltf_test.bin","byteLength":42}],
  "bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":6}],
  "accessors":[
    {"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"},
    {"bufferView":1,"componentType":5123,"count":3,"type":"SCALAR"}
  ],
  "materials":[{"pbrMetallicRoughness":{"baseColorFactor":[0.2,0.4,0.8,1],"metallicFactor":0.6,"roughnessFactor":0.25}}],
  "meshes":[{"primitives":[{"attributes":{"POSITION":0},"indices":1,"material":0}]}],
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

        cg::rt::Scene scene;
        asset.add_to(scene);
        scene.build();
        cg::rt::Hit hit;
        require(scene.intersect({{2.5, 3.5, 6.0}, {0.0, 0.0, -1.0}}, 1e-5, 10.0, hit),
                "imported glTF triangles must be usable by the ray tracer");

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

        std::filesystem::remove(gltf_path);
        std::filesystem::remove(bin_path);
        std::cout << "glTF tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::filesystem::remove(gltf_path);
        std::filesystem::remove(bin_path);
        std::cerr << "glTF tests failed: " << error.what() << '\n';
        return 1;
    }
}
