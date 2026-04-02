#include <stdio.h>
#include <musa_runtime.h>

__global__ void hello_kernel() {
    printf("Hello from MUSA GPU!\n");
}

int main() {
    printf("Testing MUSA Runtime...\n");
    
    int deviceCount;
    musaGetDeviceCount(&deviceCount);
    printf("Found %d MUSA devices\n", deviceCount);
    
    musaDeviceProp_t prop;
    musaGetDeviceProperties(&prop, 0);
    printf("Device: %s\n", prop.name);
    
    hello_kernel<<<1, 1>>>();
    musaDeviceSynchronize();
    
    printf("MUSA test passed!\n");
    return 0;
}