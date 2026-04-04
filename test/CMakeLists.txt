set(SUBSYS_NAME global_tests)
set(SUBSYS_DESC "Point cloud library global unit tests")

set(SUBSYS_DEPS common sample_consensus io kdtree geometry search octree)

set(DEFAULT OFF)
set(build TRUE)
PCL_SUBSYS_OPTION(build "${SUBSYS_NAME}" "${SUBSYS_DESC}" ${DEFAULT} "${REASON}")
PCL_SUBSYS_DEPEND(build "${SUBSYS_NAME}" DEPS ${SUBSYS_DEPS})

if(build)

    find_package(Gtest QUIET)
    if(NOT GTEST_FOUND)
      message(STATUS "GTest not found - disabling tests")
      set(build FALSE)
    endif()

endif()

if(build)

    include_directories(SYSTEM ${GTEST_INCLUDE_DIRS} ${GTEST_SRC_DIR})

    add_library(pcl_gtest STATIC ${GTEST_SRC_DIR}/src/gtest-all.cc)
  	
    if( MSVC11 )
        add_definitions("-D_VARIADIC_MAX=10")
    endif()

    enable_testing()
    include_directories(${PCL_INCLUDE_DIRS})

    add_custom_target(tests "${CMAKE_CTEST_COMMAND}" "-V" VERBATIM)

    add_subdirectory(common)
    add_subdirectory(io)
    add_subdirectory(kdtree)
    add_subdirectory(geometry)
    add_subdirectory(octree)
    add_subdirectory(search)
    add_subdirectory(sample_consensus)
    
    # Add GPU tests if GPU build is enabled
    if(BUILD_GPU)
        add_subdirectory(gpu)
    endif()

    PCL_ADD_TEST(search test_search
                 FILES test_search.cpp
                 LINK_WITH pcl_gtest pcl_search pcl_io pcl_kdtree
                 ARGUMENTS "${PCL_SOURCE_DIR}/test/table_scene_mug_stereo_textured.pcd")

    PCL_ADD_TEST(a_transforms_test test_transforms
                 FILES test_transforms.cpp
                 LINK_WITH pcl_gtest pcl_io
                 ARGUMENTS "${PCL_SOURCE_DIR}/test/bun0.pcd")

endif(build)