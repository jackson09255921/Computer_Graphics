#include "assets/gltf_loader.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("converted_asset_tests requires the mesh directory");
        const std::filesystem::path root = argv[1];
        const std::vector<std::pair<const char*, std::size_t>> expected{
            {"bench_from_asc.gltf", 448}, {"bench_from_obj.gltf", 448},
            {"bunny_from_obj.gltf", 4968}, {"cube_from_obj.gltf", 12},
            {"drop_from_asc.gltf", 264}, {"drop_from_obj.gltf", 264},
            {"glass_from_asc.gltf", 1680}, {"glass_from_obj.gltf", 1680},
            {"grid4x4_from_asc.gltf", 32}, {"quad_from_obj.gltf", 2},
            {"skull_from_asc.gltf", 2084}, {"skull_from_obj.gltf", 2084},
            {"suzanne_from_obj.gltf", 968}, {"teapot_from_obj.gltf", 6320}};
        for (const auto& [name, triangle_count] : expected) {
            const cg::assets::GltfAsset asset = cg::assets::GltfAsset::load(root / name);
            if (asset.triangles().size() != triangle_count)
                throw std::runtime_error(std::string(name) + " triangle count mismatch");
        }
        std::cout << "validated " << expected.size() << " converted glTF assets\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "converted asset tests failed: " << error.what() << '\n';
        return 1;
    }
}
