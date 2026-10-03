#include <cuda_runtime.h>

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void check(cudaError_t result, const char* operation) {
    if (result != cudaSuccess) {
        throw std::runtime_error(std::string(operation) + ": " + cudaGetErrorString(result));
    }
}

__global__ void fill(unsigned int* values, std::size_t count) {
    const std::size_t index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index < count) {
        values[index] = static_cast<unsigned int>(index ^ 0x5A5A5A5Au);
    }
}

}  // namespace

int main() {
    try {
        int device_count = 0;
        check(cudaGetDeviceCount(&device_count), "cudaGetDeviceCount");
        if (device_count == 0) {
            throw std::runtime_error("no CUDA device found");
        }

        cudaDeviceProp properties{};
        check(cudaGetDeviceProperties(&properties, 0), "cudaGetDeviceProperties");
        if (properties.major < 8) {
            throw std::runtime_error("LabX GPU path requires compute capability 8.0 or newer");
        }

        constexpr std::size_t allocation_bytes = 64u * 1024u * 1024u;
        constexpr std::size_t value_count = allocation_bytes / sizeof(unsigned int);
        unsigned int* device_values = nullptr;
        check(cudaMalloc(&device_values, allocation_bytes), "64 MiB cudaMalloc");

        constexpr unsigned int threads = 256;
        const unsigned int blocks = static_cast<unsigned int>((value_count + threads - 1) / threads);
        fill<<<blocks, threads>>>(device_values, value_count);
        check(cudaGetLastError(), "fill kernel launch");

        unsigned int endpoints[2]{};
        check(cudaMemcpy(&endpoints[0], device_values, sizeof(unsigned int), cudaMemcpyDeviceToHost),
              "copy first value");
        check(cudaMemcpy(&endpoints[1], device_values + value_count - 1, sizeof(unsigned int),
                         cudaMemcpyDeviceToHost), "copy last value");
        check(cudaFree(device_values), "cudaFree");

        const unsigned int expected_first = 0x5A5A5A5Au;
        const unsigned int expected_last = static_cast<unsigned int>((value_count - 1) ^ 0x5A5A5A5Au);
        if (endpoints[0] != expected_first || endpoints[1] != expected_last) {
            throw std::runtime_error("CUDA kernel validation failed");
        }

        const double gibibytes = static_cast<double>(properties.totalGlobalMem) / (1024.0 * 1024.0 * 1024.0);
        std::cout << properties.name << " | compute " << properties.major << '.' << properties.minor
                  << " | " << gibibytes << " GiB VRAM | 64 MiB allocation and kernel passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "CUDA capability test failed: " << error.what() << '\n';
        return 1;
    }
}
