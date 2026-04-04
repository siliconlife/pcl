/*
 * PCL GPU Stress Test - Continuous GPU Load Generator
 * 
 * This test generates sustained GPU load for visualization in task manager.
 * It continuously performs GPU-accelerated operations for a specified duration.
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>
#include <cstring>
#include <iomanip>

// PCL headers
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>

// PCL GPU headers
#include <pcl/gpu/containers/device_array.h>
#include <pcl/gpu/containers/initialization.h>
#include <pcl/gpu/octree/octree.hpp>

// Timer class
class Timer {
public:
    void start() { start_time = std::chrono::high_resolution_clock::now(); }
    void stop() { end_time = std::chrono::high_resolution_clock::now(); }
    double elapsed_ms() {
        return std::chrono::duration<double, std::milli>(end_time - start_time).count();
    }
    double elapsed_seconds() {
        return std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start_time).count();
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time, end_time;
};

// Print progress bar
void print_progress(double percentage, double elapsed_sec, double total_sec) {
    int bar_width = 50;
    int pos = static_cast<int>(bar_width * percentage / 100.0);
    
    std::cout << "\r[";
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << std::fixed << std::setprecision(1) 
              << percentage << "% | " 
              << std::setprecision(1) << elapsed_sec << "s / " 
              << std::setprecision(1) << total_sec << "s";
    std::cout.flush();
}

// Generate random point cloud
void generate_random_cloud(pcl::PointCloud<pcl::PointXYZ>& cloud, int num_points) {
    cloud.width = num_points;
    cloud.height = 1;
    cloud.points.resize(num_points);
    
    for (int i = 0; i < num_points; ++i) {
        cloud.points[i].x = static_cast<float>(rand()) / RAND_MAX * 100.0f;
        cloud.points[i].y = static_cast<float>(rand()) / RAND_MAX * 100.0f;
        cloud.points[i].z = static_cast<float>(rand()) / RAND_MAX * 100.0f;
    }
}

// GPU Octree stress test
void run_octree_stress_test(double duration_seconds) {
    std::cout << "\n=== GPU Octree Stress Test ===" << std::endl;
    std::cout << "Duration: " << duration_seconds << " seconds" << std::endl;
    std::cout << "This test continuously builds octrees on GPU..." << std::endl;
    std::cout << "Watch your task manager for GPU activity!\n" << std::endl;
    
    const int num_points = 500000;  // 500K points for good GPU utilization
    const int iterations = static_cast<int>(duration_seconds / 0.5);  // ~0.5s per iteration
    
    Timer total_timer;
    total_timer.start();
    
    int completed_iterations = 0;
    double total_build_time = 0.0;
    
    while (total_timer.elapsed_seconds() < duration_seconds) {
        // Generate random point cloud
        pcl::PointCloud<pcl::PointXYZ> host_cloud;
        generate_random_cloud(host_cloud, num_points);
        
        // Upload to GPU
        pcl::gpu::Octree::PointCloud device_cloud;
        device_cloud.upload(host_cloud.points);
        
        // Build octree on GPU
        pcl::gpu::Octree octree;
        octree.setCloud(device_cloud);
        
        Timer build_timer;
        build_timer.start();
        octree.build();
        build_timer.stop();
        
        total_build_time += build_timer.elapsed_ms();
        completed_iterations++;
        
        // Update progress
        double elapsed = total_timer.elapsed_seconds();
        double percentage = (elapsed / duration_seconds) * 100.0;
        print_progress(percentage, elapsed, duration_seconds);
        
        // Small delay to control iteration rate
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    total_timer.stop();
    std::cout << std::endl;
    
    std::cout << "\n=== Octree Stress Test Results ===" << std::endl;
    std::cout << "Completed iterations: " << completed_iterations << std::endl;
    std::cout << "Average build time: " << (total_build_time / completed_iterations) << " ms" << std::endl;
    std::cout << "Points processed: " << (completed_iterations * num_points) << std::endl;
    std::cout << "Throughput: " << (completed_iterations * num_points / total_timer.elapsed_seconds() / 1e6) 
              << " M points/s" << std::endl;
}

// GPU Memory bandwidth stress test
void run_memory_stress_test(double duration_seconds) {
    std::cout << "\n=== GPU Memory Bandwidth Stress Test ===" << std::endl;
    std::cout << "Duration: " << duration_seconds << " seconds" << std::endl;
    std::cout << "This test continuously transfers data between Host and Device...\n" << std::endl;
    
    const size_t data_size = 100 * 1024 * 1024;  // 100 MB per transfer
    const int num_iterations = static_cast<int>(duration_seconds * 10);  // ~10 iterations per second
    
    std::vector<pcl::PointXYZ> host_data(data_size / sizeof(pcl::PointXYZ));
    for (auto& pt : host_data) {
        pt.x = static_cast<float>(rand()) / RAND_MAX;
        pt.y = static_cast<float>(rand()) / RAND_MAX;
        pt.z = static_cast<float>(rand()) / RAND_MAX;
    }
    
    pcl::gpu::Octree::PointCloud device_buffer;
    device_buffer.create(data_size / sizeof(pcl::PointXYZ));
    
    Timer total_timer;
    total_timer.start();
    
    double total_h2d_time = 0.0;
    double total_d2h_time = 0.0;
    int completed_iterations = 0;
    
    while (total_timer.elapsed_seconds() < duration_seconds) {
        // H2D transfer
        Timer h2d_timer;
        h2d_timer.start();
        device_buffer.upload(host_data);
        h2d_timer.stop();
        total_h2d_time += h2d_timer.elapsed_ms();
        
        // D2H transfer
        Timer d2h_timer;
        d2h_timer.start();
        device_buffer.download(host_data);
        d2h_timer.stop();
        total_d2h_time += d2h_timer.elapsed_ms();
        
        completed_iterations++;
        
        // Update progress
        double elapsed = total_timer.elapsed_seconds();
        double percentage = (elapsed / duration_seconds) * 100.0;
        print_progress(percentage, elapsed, duration_seconds);
    }
    
    total_timer.stop();
    std::cout << std::endl;
    
    double total_data_gb = (2.0 * data_size * completed_iterations) / (1024.0 * 1024.0 * 1024.0);
    double avg_h2d_bw = (data_size * completed_iterations / 1024.0 / 1024.0 / 1024.0) / (total_h2d_time / 1000.0);
    double avg_d2h_bw = (data_size * completed_iterations / 1024.0 / 1024.0 / 1024.0) / (total_d2h_time / 1000.0);
    
    std::cout << "\n=== Memory Stress Test Results ===" << std::endl;
    std::cout << "Completed iterations: " << completed_iterations << std::endl;
    std::cout << "Total data transferred: " << std::fixed << std::setprecision(2) << total_data_gb << " GB" << std::endl;
    std::cout << "Average H2D bandwidth: " << std::setprecision(2) << avg_h2d_bw << " GB/s" << std::endl;
    std::cout << "Average D2H bandwidth: " << std::setprecision(2) << avg_d2h_bw << " GB/s" << std::endl;
}

// Combined stress test - maximum GPU utilization
void run_combined_stress_test(double duration_seconds) {
    std::cout << "\n=== Combined GPU Stress Test (Maximum Load) ===" << std::endl;
    std::cout << "Duration: " << duration_seconds << " seconds" << std::endl;
    std::cout << "This test runs octree building and memory transfers in parallel...\n" << std::endl;
    
    const int num_points = 300000;
    const size_t mem_size = 50 * 1024 * 1024;
    
    std::vector<pcl::PointXYZ> host_data(mem_size / sizeof(pcl::PointXYZ));
    pcl::gpu::Octree::PointCloud device_buffer;
    device_buffer.create(mem_size / sizeof(pcl::PointXYZ));
    
    Timer total_timer;
    total_timer.start();
    
    int octree_iterations = 0;
    int memory_iterations = 0;
    
    while (total_timer.elapsed_seconds() < duration_seconds) {
        // Alternate between octree and memory operations
        if (static_cast<int>(total_timer.elapsed_seconds() * 10) % 2 == 0) {
            // Octree operation
            pcl::PointCloud<pcl::PointXYZ> host_cloud;
            generate_random_cloud(host_cloud, num_points);
            
            pcl::gpu::Octree::PointCloud device_cloud;
            device_cloud.upload(host_cloud.points);
            
            pcl::gpu::Octree octree;
            octree.setCloud(device_cloud);
            octree.build();
            
            octree_iterations++;
        } else {
            // Memory operation
            for (auto& pt : host_data) {
                pt.x = static_cast<float>(rand()) / RAND_MAX;
                pt.y = static_cast<float>(rand()) / RAND_MAX;
                pt.z = static_cast<float>(rand()) / RAND_MAX;
            }
            
            device_buffer.upload(host_data);
            device_buffer.download(host_data);
            
            memory_iterations++;
        }
        
        // Update progress
        double elapsed = total_timer.elapsed_seconds();
        double percentage = (elapsed / duration_seconds) * 100.0;
        print_progress(percentage, elapsed, duration_seconds);
    }
    
    total_timer.stop();
    std::cout << std::endl;
    
    std::cout << "\n=== Combined Stress Test Results ===" << std::endl;
    std::cout << "Octree iterations: " << octree_iterations << std::endl;
    std::cout << "Memory iterations: " << memory_iterations << std::endl;
    std::cout << "Total operations: " << (octree_iterations + memory_iterations) << std::endl;
    std::cout << "Average ops/sec: " << ((octree_iterations + memory_iterations) / total_timer.elapsed_seconds()) << std::endl;
}

// Main function
int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "PCL GPU Stress Test - Task Manager Demo" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // Check GPU availability
    int device_count = pcl::gpu::getCudaEnabledDeviceCount();
    if (device_count <= 0) {
        std::cerr << "ERROR: No GPU devices available!" << std::endl;
        std::cerr << "This test requires GPU support." << std::endl;
        return 1;
    }
    
    std::cout << "GPU Devices Found: " << device_count << std::endl;
    std::cout << "Device 0: " << pcl::gpu::getDeviceName(0) << std::endl;
    std::cout << std::endl;
    
    // Parse duration argument
    double duration = 30.0;  // Default 30 seconds
    if (argc > 1) {
        duration = std::atof(argv[1]);
        if (duration <= 0) duration = 30.0;
    }
    
    std::cout << "Test duration: " << duration << " seconds" << std::endl;
    std::cout << "Open your task manager/system monitor to observe GPU activity!" << std::endl;
    std::cout << std::endl;
    
    // Set GPU device
    pcl::gpu::setDevice(0);
    
    // Run stress tests
    run_octree_stress_test(duration);
    run_memory_stress_test(duration / 3);
    run_combined_stress_test(duration);
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "All stress tests completed!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
