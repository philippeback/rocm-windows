#include <hip/hip_runtime.h>
#include <cstdlib>
#include <iostream>

#if !defined(__HIP_DEVICE_COMPILE__)
#define HIP_CHECK(call)                                                          \
    do {                                                                        \
        const hipError_t err = (call);                                           \
        if (err != hipSuccess) {                                                 \
            std::cerr << "HIP error: " << hipGetErrorString(err)                 \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(EXIT_FAILURE);                                            \
        }                                                                       \
    } while (0)
#else
#define HIP_CHECK(call) do { (void)(call); } while (0)
#endif

__global__ void vectorAdd(const float* a, const float* b, float* c, int n) {
    int i = blockDim.x * blockIdx.x + threadIdx.x;
    if (i < n) {
        c[i] = a[i] + b[i];
    }
}

#if defined(_WIN32) && !defined(__HIP_DEVICE_COMPILE__)
extern "C" __declspec(dllimport) int __stdcall SetConsoleOutputCP(unsigned int wCodePageID);
#endif

#if !defined(__HIP_DEVICE_COMPILE__)
int main() {
#if defined(_WIN32) && !defined(__HIP_DEVICE_COMPILE__)
    SetConsoleOutputCP(65001); // CP_UTF8
#endif
    const int N = 1000000;
    size_t bytes = N * sizeof(float);

    float *h_a = new float[N];
    float *h_b = new float[N];
    float *h_c = new float[N];

    for (int i = 0; i < N; i++) {
        h_a[i] = 1.5f;
        h_b[i] = 2.5f;
    }

    float *d_a, *d_b, *d_c;
    HIP_CHECK(hipMalloc(&d_a, bytes));
    HIP_CHECK(hipMalloc(&d_b, bytes));
    HIP_CHECK(hipMalloc(&d_c, bytes));

    HIP_CHECK(hipMemcpy(d_a, h_a, bytes, hipMemcpyHostToDevice));
    HIP_CHECK(hipMemcpy(d_b, h_b, bytes, hipMemcpyHostToDevice));

    int blockSize = 256;
    int gridSize = (N + blockSize - 1) / blockSize;

    vectorAdd<<<gridSize, blockSize>>>(d_a, d_b, d_c, N);
    HIP_CHECK(hipDeviceSynchronize());

    HIP_CHECK(hipMemcpy(h_c, d_c, bytes, hipMemcpyDeviceToHost));

    bool success = true;
    for (int i = 0; i < N; i++) {
        if (h_c[i] != 4.0f) {
            success = false;
            break;
        }
    }

    std::cout << "Calcul HIP sur 8060S (gfx1151) : "
              << (success ? "SUCCÈS (1M éléments validés)" : "ÉCHEC")
              << std::endl;

    hipFree(d_a);
    hipFree(d_b);
    hipFree(d_c);
    delete[] h_a;
    delete[] h_b;
    delete[] h_c;

    return 0;
}
#endif