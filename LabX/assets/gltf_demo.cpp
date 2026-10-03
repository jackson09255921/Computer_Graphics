#include <exception>
#include <filesystem>
#include <iostream>

#include "assets/gltf_loader.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: gltf_demo model.gltf\n";
        return 2;
    }
    try {
        const cg::assets::GltfAsset asset = cg::assets::GltfAsset::load(std::filesystem::path(argv[1]));
        std::cout << "loaded " << asset.triangles().size() << " triangles from " << argv[1] << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "glTF load failed: " << error.what() << '\n';
        return 1;
    }
}
