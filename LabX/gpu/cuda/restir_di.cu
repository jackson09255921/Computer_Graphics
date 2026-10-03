#include <cuda_runtime.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void check(cudaError_t result, const char* operation) {
    if (result != cudaSuccess) throw std::runtime_error(std::string(operation) + ": " + cudaGetErrorString(result));
}

struct Rng {
    std::uint32_t state;
    __device__ explicit Rng(std::uint32_t seed) : state(seed ? seed : 1u) {}
    __device__ float next() {
        state ^= state << 13u;
        state ^= state >> 17u;
        state ^= state << 5u;
        return static_cast<float>(state) * 2.3283064365386963e-10f;
    }
};

struct Candidate {
    int light_id;
    float target;
    float proposal_pdf;
};

struct Reservoir {
    Candidate selected{-1, 0.0f, 0.0f};
    float weight_sum{0.0f};
    unsigned int sample_count{0};
};

__device__ void update(Reservoir& reservoir, Candidate candidate, float weight,
                       unsigned int represented_samples, Rng& rng) {
    if (!(weight > 0.0f) || represented_samples == 0) return;
    reservoir.weight_sum += weight;
    reservoir.sample_count += represented_samples;
    if (rng.next() * reservoir.weight_sum < weight) reservoir.selected = candidate;
}

__device__ void stream_candidate(Reservoir& reservoir, Candidate candidate, Rng& rng) {
    if (!(candidate.target > 0.0f) || !(candidate.proposal_pdf > 0.0f)) return;
    update(reservoir, candidate, candidate.target / candidate.proposal_pdf, 1, rng);
}

__device__ void merge(Reservoir& destination, const Reservoir& source, Rng& rng) {
    if (source.sample_count == 0 || source.selected.light_id < 0) return;
    update(destination, source.selected, source.weight_sum, source.sample_count, rng);
}

__device__ float normalization(const Reservoir& reservoir) {
    if (reservoir.sample_count == 0 || !(reservoir.selected.target > 0.0f)) return 0.0f;
    return reservoir.weight_sum /
           (static_cast<float>(reservoir.sample_count) * reservoir.selected.target);
}

struct ProbeResult {
    Reservoir initial;
    Reservoir temporal;
    Reservoir spatial;
    float normalization;
};

__global__ void reservoir_probe(ProbeResult* result) {
    Rng rng(0x5EEDu);
    Reservoir initial;
    stream_candidate(initial, {0, 1.0f, 1.0f}, rng);
    stream_candidate(initial, {1, 3.0f, 1.0f}, rng);
    stream_candidate(initial, {2, 6.0f, 1.0f}, rng);

    Reservoir history;
    stream_candidate(history, {3, 2.0f, 1.0f}, rng);
    stream_candidate(history, {4, 2.0f, 1.0f}, rng);
    Reservoir temporal = initial;
    merge(temporal, history, rng);

    Reservoir neighbor;
    stream_candidate(neighbor, {5, 5.0f, 1.0f}, rng);
    Reservoir spatial = temporal;
    merge(spatial, neighbor, rng);

    result->initial = initial;
    result->temporal = temporal;
    result->spatial = spatial;
    result->normalization = normalization(spatial);
}

void self_test() {
    ProbeResult* device_result = nullptr;
    try {
        check(cudaMalloc(&device_result, sizeof(ProbeResult)), "cudaMalloc ReSTIR probe");
        reservoir_probe<<<1, 1>>>(device_result);
        check(cudaGetLastError(), "ReSTIR reservoir probe launch");
        ProbeResult result{};
        check(cudaMemcpy(&result, device_result, sizeof(result), cudaMemcpyDeviceToHost),
              "copy ReSTIR reservoir probe");
        check(cudaFree(device_result), "cudaFree ReSTIR probe");
        device_result = nullptr;
        const bool valid_ids = result.initial.selected.light_id >= 0 && result.initial.selected.light_id <= 2 &&
                               result.temporal.selected.light_id >= 0 && result.temporal.selected.light_id <= 4 &&
                               result.spatial.selected.light_id >= 0 && result.spatial.selected.light_id <= 5;
        if (!valid_ids || result.initial.sample_count != 3 || result.temporal.sample_count != 5 ||
            result.spatial.sample_count != 6 || fabsf(result.initial.weight_sum - 10.0f) > 1.0e-6f ||
            fabsf(result.temporal.weight_sum - 14.0f) > 1.0e-6f ||
            fabsf(result.spatial.weight_sum - 19.0f) > 1.0e-6f ||
            !std::isfinite(result.normalization) || result.normalization <= 0.0f)
            throw std::runtime_error("ReSTIR reservoir invariants failed");
        std::cout << "ReSTIR DI reservoir update, temporal reuse, and spatial reuse passed\n";
    } catch (...) {
        if (device_result) cudaFree(device_result);
        throw;
    }
}

}  // namespace

int main() {
    try {
        self_test();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ReSTIR DI test failed: " << error.what() << '\n';
        return 1;
    }
}
