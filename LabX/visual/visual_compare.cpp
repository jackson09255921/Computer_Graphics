#include "visual/visual_regression.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    try {
        if (argc < 4 || argc > 6) {
            std::cerr << "usage: visual_compare reference.(bmp|ppm) actual.(bmp|ppm) diff.ppm [max-mae] [max-rmse]\n";
            return 2;
        }
        const double maximum_mae = argc > 4 ? std::stod(argv[4]) : 0.0;
        const double maximum_rmse = argc > 5 ? std::stod(argv[5]) : maximum_mae;
        const cg::visual::Image reference = cg::visual::Image::read(argv[1]);
        const cg::visual::Image actual = cg::visual::Image::read(argv[2]);
        const cg::visual::Metrics metrics = cg::visual::compare(reference, actual);
        cg::visual::difference_heatmap(reference, actual).write_ppm(argv[3]);
        std::cout << "visual metrics: MAE=" << metrics.mae << " RMSE=" << metrics.rmse
                  << " max=" << static_cast<unsigned int>(metrics.maximum_error)
                  << " changed_channels=" << metrics.changed_channels << '\n';
        return metrics.mae <= maximum_mae && metrics.rmse <= maximum_rmse ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "visual comparison failed: " << error.what() << '\n';
        return 2;
    }
}
