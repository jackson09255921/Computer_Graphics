#pragma once

#include <cuda_runtime.h>
#include <optix.h>

struct OptixTriangleParams {
    uchar4* image;
    unsigned int width;
    unsigned int height;
    OptixTraversableHandle handle;
};

