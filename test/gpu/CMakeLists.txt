# PCL GPU MUSA Test Suite
# This file is included from test/CMakeLists.txt when building PCL

# GPU 测试 - 仅在启用 GPU 构建时添加
if(BUILD_GPU OR WITH_MUSA)
    message(STATUS "Adding GPU tests")
    
    # 包含目录
    include_directories(
        ${PCL_SOURCE_DIR}/common/include
        ${PCL_SOURCE_DIR}/gpu/containers/include
        ${PCL_SOURCE_DIR}/gpu/utils/include
    )
    
    # 如果启用了 MUSA，添加 MUSA 包含目录
    if(WITH_MUSA)
        include_directories(/usr/local/musa/include)
        link_directories(/usr/local/musa/lib)
    endif()
    
    # 简化版 GPU 测试（纯 CPU 模拟，用于验证基本功能）
    if(BUILD_global_tests)
        PCL_ADD_TEST(gpu_simple_test test_gpu_simple
                     FILES test_gpu_simple.cpp
                     LINK_WITH pcl_gtest pcl_common
                     ARGUMENTS "${PCL_SOURCE_DIR}")
        
        # 完整 GPU 测试（需要 MUSA 运行时）
        # 注意：这个测试需要 mcc 编译器支持
        # if(WITH_MUSA AND MUSA_FOUND)
        #     PCL_ADD_TEST(gpu_musa_test test_gpu_musa
        #                  FILES test_gpu_musa.cpp
        #                  LINK_WITH pcl_gtest pcl_common pcl_gpu_containers musart musa
        #                  ARGUMENTS "${PCL_SOURCE_DIR}")
        # endif()
    endif()
endif()
