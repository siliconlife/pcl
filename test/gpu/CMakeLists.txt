# PCL GPU MUSA Test Suite
# This file is included from test/CMakeLists.txt when building PCL

# GPU 测试 - 仅在启用 GPU 构建时添加
if(BUILD_GPU)
    message(STATUS "Adding GPU tests")
    
    include_directories(
        ${PCL_SOURCE_DIR}/common/include
        ${PCL_SOURCE_DIR}/gpu/containers/include
        ${PCL_SOURCE_DIR}/gpu/utils/include
        ${PCL_SOURCE_DIR}/gpu/octree/include
    )
    
    if(WITH_MUSA)
        include_directories(/usr/local/musa/include)
        link_directories(/usr/local/musa/lib)
    endif()
    
    if(BUILD_global_tests)
        PCL_ADD_TEST(gpu_simple_test test_gpu_simple
                     FILES test_gpu_simple.cpp
                     LINK_WITH pcl_gtest pcl_common
                     ARGUMENTS "${PCL_SOURCE_DIR}")
        
        PCL_ADD_TEST(gpu_stress_test test_gpu_stress
                     FILES test_gpu_stress.cpp
                     LINK_WITH pcl_gtest pcl_common pcl_gpu_containers pcl_gpu_octree
                     ARGUMENTS "${PCL_SOURCE_DIR}")
    endif()
endif()
