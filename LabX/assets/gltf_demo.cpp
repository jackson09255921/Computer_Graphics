#include <algorithm>
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
        const std::size_t smooth = static_cast<std::size_t>(std::count_if(
            asset.triangles().begin(), asset.triangles().end(),
            [](const cg::assets::GltfTriangle& triangle) { return triangle.has_normals; }));
        std::cout << "loaded " << asset.triangles().size() << " triangles (" << smooth
                  << " with vertex normals) from " << argv[1] << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "glTF load failed: " << error.what() << '\n';
        return 1;
    }
}
