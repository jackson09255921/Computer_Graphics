#include "assets/asc_loader.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

int main() {
    const std::filesystem::path path = "asc-loader-test.asc";
    try {
        {
            std::ofstream output(path);
            output << "4 1\n0 0 0\n1 0 0\n1 1 0\n0 1 0\n4 1 2 3 4\n";
        }
        const cg::assets::AscMesh mesh = cg::assets::AscMesh::load(path);
        if (mesh.vertices.size() != 4 || mesh.triangles.size() != 2 ||
            mesh.triangles[1].first != 0 || mesh.triangles[1].second != 2 || mesh.triangles[1].third != 3)
            throw std::runtime_error("ASC fan triangulation failed");
        std::filesystem::remove(path);
        std::cout << "ASC loader tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::filesystem::remove(path);
        std::cerr << "ASC loader tests failed: " << error.what() << '\n';
        return 1;
    }
}
