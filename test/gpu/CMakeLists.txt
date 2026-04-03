cmake_minimum_required(VERSION 3.5)

# PCL GPU MUSA Test Suite CMakeLists.txt

project(pcl_gpu_tests LANGUAGES CXX)

# 查找 PCL
find_package(PCL REQUIRED)
include_directories(${PCL_INCLUDE_DIRS})
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../gpu/containers/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../gpu/utils/include
    /usr/local/musa/include
)

# 链接目录
link_directories(/usr/local/musa/lib)

# 测试可执行文件
add_executable(test_gpu_musa test_gpu_musa.cpp)

# 设置编译选项
target_compile_options(test_gpu_musa PRIVATE 
    -DHAVE_MUSA 
    -DHAVE_CUDA
    -std=c++14
)

# 链接库
target_link_libraries(test_gpu_musa
    ${PCL_LIBRARIES}
    musart
    musa
    pcl_gpu_containers
    pcl_common
)

# 添加测试
add_test(NAME test_gpu_musa COMMAND test_gpu_musa)
set_tests_properties(test_gpu_musa PROPERTIES
    TIMEOUT 300
    LABELS "gpu;musa"
)
