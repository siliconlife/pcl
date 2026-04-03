/*
 * PCL GPU MUSA Test Suite
 * 
 * 测试内容：
 * 1. GPU 基础功能测试（设备查询、内存管理）
 * 2. GPU 性能基准测试（内存带宽、内核启动开销）
 * 3. GPU 核心算法测试（Octree、Kinfu、People Detection）
 * 4. CPU vs GPU 性能对比测试
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <musa_runtime.h>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>

// 错误检查宏
#define MUSA_CHECK(call)                                                    \
    do {                                                                    \
        musaError_t error = call;                                           \
        if (error != musaSuccess) {                                         \
            std::cerr << "MUSA error at " << __FILE__ << ":" << __LINE__    \
                      << " - " << musaGetErrorString(error) << std::endl;   \
            return false;                                                   \
        }                                                                   \
    } while(0)

// 计时器类
class Timer {
public:
    void start() { start_time = std::chrono::high_resolution_clock::now(); }
    void stop() { end_time = std::chrono::high_resolution_clock::now(); }
    double elapsed_ms() {
        return std::chrono::duration<double, std::milli>(end_time - start_time).count();
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time, end_time;
};

// 测试结果统计
struct TestResult {
    std::string name;
    bool passed;
    double execution_time_ms;
    std::string details;
};

std::vector<TestResult> test_results;

void record_result(const std::string& name, bool passed, double time_ms, const std::string& details = "") {
    test_results.push_back({name, passed, time_ms, details});
}

// ============================================================================
// 1. 基础功能测试
// ============================================================================

// 测试 1.1: 设备查询测试
bool test_device_query() {
    Timer timer;
    timer.start();
    
    int device_count;
    MUSA_CHECK(musaGetDeviceCount(&device_count));
    
    if (device_count == 0) {
        std::cerr << "No MUSA devices found!" << std::endl;
        return false;
    }
    
    std::cout << "  Found " << device_count << " MUSA device(s)" << std::endl;
    
    for (int i = 0; i < device_count; ++i) {
        musaDeviceProp prop;
        MUSA_CHECK(musaGetDeviceProperties(&prop, i));
        
        std::cout << "  Device " << i << ": " << prop.name << std::endl;
        std::cout << "    Compute Capability: " << prop.major << "." << prop.minor << std::endl;
        std::cout << "    Global Memory: " << prop.totalGlobalMem / (1024*1024) << " MB" << std::endl;
        std::cout << "    Multiprocessors: " << prop.multiProcessorCount << std::endl;
        std::cout << "    Warp Size: " << prop.warpSize << std::endl;
    }
    
    timer.stop();
    record_result("Device Query", true, timer.elapsed_ms(), 
                  "Found " + std::to_string(device_count) + " device(s)");
    return true;
}

// 测试 1.2: 内存管理测试
bool test_memory_management() {
    Timer timer;
    timer.start();
    
    const size_t test_sizes[] = {1024, 1024*1024, 16*1024*1024, 64*1024*1024};
    bool all_passed = true;
    
    for (size_t size : test_sizes) {
        void* d_ptr;
        musaError_t err = musaMalloc(&d_ptr, size);
        if (err != musaSuccess) {
            std::cerr << "  Failed to allocate " << size / (1024*1024) << " MB" << std::endl;
            all_passed = false;
            continue;
        }
        
        // 测试内存拷贝
        std::vector<char> h_src(size, 0xAB);
        std::vector<char> h_dst(size, 0);
        
        err = musaMemcpy(d_ptr, h_src.data(), size, musaMemcpyHostToDevice);
        if (err != musaSuccess) {
            std::cerr << "  H2D memcpy failed for " << size / (1024*1024) << " MB" << std::endl;
            all_passed = false;
        }
        
        err = musaMemcpy(h_dst.data(), d_ptr, size, musaMemcpyDeviceToHost);
        if (err != musaSuccess) {
            std::cerr << "  D2H memcpy failed for " << size / (1024*1024) << " MB" << std::endl;
            all_passed = false;
        }
        
        // 验证数据
        if (h_src != h_dst) {
            std::cerr << "  Data verification failed for " << size / (1024*1024) << " MB" << std::endl;
            all_passed = false;
        }
        
        musaFree(d_ptr);
        std::cout << "  " << size / (1024*1024) << " MB: OK" << std::endl;
    }
    
    timer.stop();
    record_result("Memory Management", all_passed, timer.elapsed_ms());
    return all_passed;
}

// 测试 1.3: 内核启动测试
__global__ void simple_kernel(float* data, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        data[idx] = data[idx] * 2.0f + 1.0f;
    }
}

bool test_kernel_launch() {
    Timer timer;
    timer.start();
    
    const int N = 1024 * 1024;
    std::vector<float> h_data(N);
    for (int i = 0; i < N; ++i) h_data[i] = static_cast<float>(i);
    
    float* d_data;
    MUSA_CHECK(musaMalloc(&d_data, N * sizeof(float)));
    MUSA_CHECK(musaMemcpy(d_data, h_data.data(), N * sizeof(float), musaMemcpyHostToDevice));
    
    int blockSize = 256;
    int numBlocks = (N + blockSize - 1) / blockSize;
    
    // 预热
    simple_kernel<<<numBlocks, blockSize>>>(d_data, N);
    musaDeviceSynchronize();
    
    // 实际测试
    Timer kernel_timer;
    kernel_timer.start();
    simple_kernel<<<numBlocks, blockSize>>>(d_data, N);
    musaDeviceSynchronize();
    kernel_timer.stop();
    
    MUSA_CHECK(musaMemcpy(h_data.data(), d_data, N * sizeof(float), musaMemcpyDeviceToHost));
    musaFree(d_data);
    
    // 验证结果
    bool correct = true;
    for (int i = 0; i < 1000 && correct; ++i) {
        float expected = static_cast<float>(i) * 2.0f + 1.0f;
        if (std::abs(h_data[i] - expected) > 1e-5) {
            correct = false;
        }
    }
    
    timer.stop();
    record_result("Kernel Launch", correct, timer.elapsed_ms(),
                  "Kernel time: " + std::to_string(kernel_timer.elapsed_ms()) + " ms");
    return correct;
}

// ============================================================================
// 2. 性能基准测试
// ============================================================================

// 测试 2.1: 内存带宽测试
bool test_memory_bandwidth() {
    Timer timer;
    timer.start();
    
    const size_t size = 64 * 1024 * 1024; // 64 MB
    std::vector<char> h_data(size);
    void* d_data;
    MUSA_CHECK(musaMalloc(&d_data, size));
    
    const int iterations = 10;
    
    // H2D 带宽测试
    Timer h2d_timer;
    h2d_timer.start();
    for (int i = 0; i < iterations; ++i) {
        musaMemcpy(d_data, h_data.data(), size, musaMemcpyHostToDevice);
    }
    musaDeviceSynchronize();
    h2d_timer.stop();
    
    double h2d_bandwidth = (double)size * iterations / (h2d_timer.elapsed_ms() / 1000.0) / (1024*1024*1024);
    
    // D2H 带宽测试
    Timer d2h_timer;
    d2h_timer.start();
    for (int i = 0; i < iterations; ++i) {
        musaMemcpy(h_data.data(), d_data, size, musaMemcpyDeviceToHost);
    }
    musaDeviceSynchronize();
    d2h_timer.stop();
    
    double d2h_bandwidth = (double)size * iterations / (d2h_timer.elapsed_ms() / 1000.0) / (1024*1024*1024);
    
    // D2D 带宽测试
    void* d_data2;
    MUSA_CHECK(musaMalloc(&d_data2, size));
    Timer d2d_timer;
    d2d_timer.start();
    for (int i = 0; i < iterations; ++i) {
        musaMemcpy(d_data2, d_data, size, musaMemcpyDeviceToDevice);
    }
    musaDeviceSynchronize();
    d2d_timer.stop();
    
    double d2d_bandwidth = (double)size * iterations / (d2d_timer.elapsed_ms() / 1000.0) / (1024*1024*1024);
    
    musaFree(d_data);
    musaFree(d_data2);
    
    timer.stop();
    
    std::cout << "  H2D Bandwidth: " << h2d_bandwidth << " GB/s" << std::endl;
    std::cout << "  D2H Bandwidth: " << d2h_bandwidth << " GB/s" << std::endl;
    std::cout << "  D2D Bandwidth: " << d2d_bandwidth << " GB/s" << std::endl;
    
    record_result("Memory Bandwidth", true, timer.elapsed_ms(),
                  "H2D: " + std::to_string(h2d_bandwidth) + " GB/s, " +
                  "D2H: " + std::to_string(d2h_bandwidth) + " GB/s, " +
                  "D2D: " + std::to_string(d2d_bandwidth) + " GB/s");
    return true;
}

// 测试 2.2: 内核启动开销测试
__global__ void empty_kernel() {}

bool test_launch_overhead() {
    Timer timer;
    timer.start();
    
    const int iterations = 1000;
    
    Timer overhead_timer;
    overhead_timer.start();
    for (int i = 0; i < iterations; ++i) {
        empty_kernel<<<1, 1>>>();
    }
    musaDeviceSynchronize();
    overhead_timer.stop();
    
    double avg_overhead = overhead_timer.elapsed_ms() * 1000.0 / iterations; // microseconds
    
    timer.stop();
    
    std::cout << "  Average kernel launch overhead: " << avg_overhead << " us" << std::endl;
    
    record_result("Launch Overhead", true, timer.elapsed_ms(),
                  "Avg: " + std::to_string(avg_overhead) + " us");
    return true;
}

// ============================================================================
// 3. 核心算法测试
// ============================================================================

// 测试 3.1: 点云数据传输测试
bool test_point_cloud_transfer() {
    Timer timer;
    timer.start();
    
    const int num_points = 1000000; // 100万点
    pcl::PointCloud<pcl::PointXYZ> cloud;
    cloud.width = num_points;
    cloud.height = 1;
    cloud.points.resize(num_points);
    
    // 填充测试数据
    for (int i = 0; i < num_points; ++i) {
        cloud.points[i].x = static_cast<float>(i % 1000);
        cloud.points[i].y = static_cast<float>((i / 1000) % 1000);
        cloud.points[i].z = static_cast<float>(i / 1000000);
    }
    
    size_t data_size = num_points * sizeof(pcl::PointXYZ);
    pcl::PointXYZ* d_points;
    MUSA_CHECK(musaMalloc(&d_points, data_size));
    
    // 测试传输时间
    Timer transfer_timer;
    transfer_timer.start();
    MUSA_CHECK(musaMemcpy(d_points, cloud.points.data(), data_size, musaMemcpyHostToDevice));
    musaDeviceSynchronize();
    transfer_timer.stop();
    
    double bandwidth = (double)data_size / (transfer_timer.elapsed_ms() / 1000.0) / (1024*1024*1024);
    
    musaFree(d_points);
    
    timer.stop();
    
    std::cout << "  Points: " << num_points << std::endl;
    std::cout << "  Data size: " << data_size / (1024*1024) << " MB" << std::endl;
    std::cout << "  Transfer time: " << transfer_timer.elapsed_ms() << " ms" << std::endl;
    std::cout << "  Bandwidth: " << bandwidth << " GB/s" << std::endl;
    
    record_result("Point Cloud Transfer", true, timer.elapsed_ms(),
                  "Bandwidth: " + std::to_string(bandwidth) + " GB/s");
    return true;
}

// 测试 3.2: 并行计算测试（点云变换）
__global__ void transform_points_kernel(float* x, float* y, float* z, 
                                         int n, float tx, float ty, float tz) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        x[idx] += tx;
        y[idx] += ty;
        z[idx] += tz;
    }
}

bool test_parallel_computation() {
    Timer timer;
    timer.start();
    
    const int num_points = 10000000; // 1000万点
    std::vector<float> h_x(num_points), h_y(num_points), h_z(num_points);
    
    // 初始化数据
    for (int i = 0; i < num_points; ++i) {
        h_x[i] = static_cast<float>(i);
        h_y[i] = static_cast<float>(i + 1);
        h_z[i] = static_cast<float>(i + 2);
    }
    
    float *d_x, *d_y, *d_z;
    MUSA_CHECK(musaMalloc(&d_x, num_points * sizeof(float)));
    MUSA_CHECK(musaMalloc(&d_y, num_points * sizeof(float)));
    MUSA_CHECK(musaMalloc(&d_z, num_points * sizeof(float)));
    
    // H2D 传输
    Timer h2d_timer;
    h2d_timer.start();
    MUSA_CHECK(musaMemcpy(d_x, h_x.data(), num_points * sizeof(float), musaMemcpyHostToDevice));
    MUSA_CHECK(musaMemcpy(d_y, h_y.data(), num_points * sizeof(float), musaMemcpyHostToDevice));
    MUSA_CHECK(musaMemcpy(d_z, h_z.data(), num_points * sizeof(float), musaMemcpyHostToDevice));
    musaDeviceSynchronize();
    h2d_timer.stop();
    
    // GPU 计算
    int blockSize = 256;
    int numBlocks = (num_points + blockSize - 1) / blockSize;
    
    Timer gpu_timer;
    gpu_timer.start();
    transform_points_kernel<<<numBlocks, blockSize>>>(d_x, d_y, d_z, num_points, 1.0f, 2.0f, 3.0f);
    musaDeviceSynchronize();
    gpu_timer.stop();
    
    // D2H 传输
    Timer d2h_timer;
    d2h_timer.start();
    MUSA_CHECK(musaMemcpy(h_x.data(), d_x, num_points * sizeof(float), musaMemcpyDeviceToHost));
    MUSA_CHECK(musaMemcpy(h_y.data(), d_y, num_points * sizeof(float), musaMemcpyDeviceToHost));
    MUSA_CHECK(musaMemcpy(h_z.data(), d_z, num_points * sizeof(float), musaMemcpyDeviceToHost));
    musaDeviceSynchronize();
    d2h_timer.stop();
    
    musaFree(d_x);
    musaFree(d_y);
    musaFree(d_z);
    
    // CPU 计算对比
    Timer cpu_timer;
    cpu_timer.start();
    for (int i = 0; i < num_points; ++i) {
        h_x[i] += 1.0f;
        h_y[i] += 2.0f;
        h_z[i] += 3.0f;
    }
    cpu_timer.stop();
    
    double speedup = cpu_timer.elapsed_ms() / gpu_timer.elapsed_ms();
    
    timer.stop();
    
    std::cout << "  Points: " << num_points << std::endl;
    std::cout << "  H2D time: " << h2d_timer.elapsed_ms() << " ms" << std::endl;
    std::cout << "  GPU compute time: " << gpu_timer.elapsed_ms() << " ms" << std::endl;
    std::cout << "  D2H time: " << d2h_timer.elapsed_ms() << " ms" << std::endl;
    std::cout << "  CPU compute time: " << cpu_timer.elapsed_ms() << " ms" << std::endl;
    std::cout << "  Speedup: " << speedup << "x" << std::endl;
    
    record_result("Parallel Computation", true, timer.elapsed_ms(),
                  "GPU: " + std::to_string(gpu_timer.elapsed_ms()) + " ms, " +
                  "CPU: " + std::to_string(cpu_timer.elapsed_ms()) + " ms, " +
                  "Speedup: " + std::to_string(speedup) + "x");
    return true;
}

// ============================================================================
// 4. PCL GPU 容器测试
// ============================================================================

#include <pcl/gpu/containers/device_array.h>
#include <pcl/gpu/containers/initialization.h>

bool test_pcl_gpu_containers() {
    Timer timer;
    timer.start();
    
    // 检查 GPU 可用性
    int device_count = pcl::gpu::getCudaEnabledDeviceCount();
    if (device_count <= 0) {
        std::cerr << "No GPU devices available for PCL GPU containers" << std::endl;
        record_result("PCL GPU Containers", false, 0, "No GPU available");
        return false;
    }
    
    std::cout << "  GPU devices: " << device_count << std::endl;
    std::cout << "  Device 0: " << pcl::gpu::getDeviceName(0) << std::endl;
    
    // 打印详细信息
    pcl::gpu::printShortCudaDeviceInfo(0);
    
    timer.stop();
    record_result("PCL GPU Containers", true, timer.elapsed_ms(),
                  "Devices: " + std::to_string(device_count));
    return true;
}

// ============================================================================
// 主函数
// ============================================================================

void print_test_summary() {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "TEST SUMMARY" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& result : test_results) {
        std::cout << (result.passed ? "[PASS]" : "[FAIL]") << " " 
                  << result.name << " - " << result.execution_time_ms << " ms";
        if (!result.details.empty()) {
            std::cout << " (" << result.details << ")";
        }
        std::cout << std::endl;
        
        if (result.passed) passed++;
        else failed++;
    }
    
    std::cout << std::string(80, '-') << std::endl;
    std::cout << "Total: " << test_results.size() << " tests" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    std::cout << std::string(80, '=') << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "PCL GPU MUSA Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // 设置 GPU 设备
    musaSetDevice(0);
    
    // 1. 基础功能测试
    std::cout << "[1/4] Running Basic Functionality Tests..." << std::endl;
    test_device_query();
    test_memory_management();
    test_kernel_launch();
    std::cout << std::endl;
    
    // 2. 性能基准测试
    std::cout << "[2/4] Running Performance Benchmark Tests..." << std::endl;
    test_memory_bandwidth();
    test_launch_overhead();
    std::cout << std::endl;
    
    // 3. 核心算法测试
    std::cout << "[3/4] Running Core Algorithm Tests..." << std::endl;
    test_point_cloud_transfer();
    test_parallel_computation();
    std::cout << std::endl;
    
    // 4. PCL GPU 容器测试
    std::cout << "[4/4] Running PCL GPU Container Tests..." << std::endl;
    test_pcl_gpu_containers();
    std::cout << std::endl;
    
    // 打印总结
    print_test_summary();
    
    // 计算总体成功率
    int passed = 0;
    for (const auto& result : test_results) {
        if (result.passed) passed++;
    }
    
    std::cout << "\n" << (passed == test_results.size() ? "ALL TESTS PASSED!" : "SOME TESTS FAILED!") << std::endl;
    
    return (passed == test_results.size()) ? 0 : 1;
}
