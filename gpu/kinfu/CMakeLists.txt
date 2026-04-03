set(SUBSYS_NAME gpu_kinfu)
set(SUBSYS_PATH gpu/kinfu)
set(SUBSYS_DESC "Kinect Fusion implementation (MUSA)")
set(SUBSYS_DEPS common io gpu_containers geometry search)

set(build FALSE)

if(NOT WITH_OPENNI)
    set(DEFAULT FALSE)
    set(REASON "OpenNI was not found or was disabled by the user.")
else()
    set(DEFAULT TRUE)
    set(REASON)
endif()

PCL_SUBSYS_OPTION(build "${SUBSYS_NAME}" "${SUBSYS_DESC}" ${DEFAULT} "${REASON}")
PCL_SUBSYS_DEPEND(build "${SUBSYS_NAME}" DEPS ${SUBSYS_DEPS})
PCL_SET_SUBSYS_INCLUDE_DIR("${SUBSYS_NAME}" "${SUBSYS_PATH}")
mark_as_advanced("BUILD_${SUBSYS_NAME}")

if (build)
    set(MUSA_ARCH_FLAG "--offload-arch=mp_22")
    set(MUSA_LIB_DIR "/usr/local/musa/lib")
    
    FILE(GLOB incs include/pcl/gpu/kinfu/*.h*)
    FILE(GLOB srcs src/*.cpp src/*.h*)
    FILE(GLOB cuda src/cuda/*.cu src/cuda/*.h*)

    source_group("Source Files\\cuda" FILES ${cuda} )
    source_group("Source Files" FILES ${srcs} )

    set(LIB_NAME "pcl_${SUBSYS_NAME}")
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include" 
                       "${CMAKE_CURRENT_SOURCE_DIR}/src" 
                       "${MUSA_LIB_DIR}/../include")

    if (UNIX OR APPLE)
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    endif()

    if(NOT UNIX OR APPLE)
        add_definitions(-DPCLAPI_EXPORTS)
    endif()

    PCL_CUDA_ADD_LIBRARY("${LIB_NAME}" "${SUBSYS_NAME}" ${srcs} ${incs} ${cuda})
    target_link_libraries("${LIB_NAME}" 
        pcl_gpu_containers 
        ${MUSA_LIB_DIR}/libmusart.so
    )
    target_link_directories("${LIB_NAME}" PRIVATE ${MUSA_LIB_DIR})

    set(EXT_DEPS "")
    PCL_MAKE_PKGCONFIG("${LIB_NAME}" "${SUBSYS_NAME}" "${SUBSYS_DESC}" "${SUBSYS_DEPS}" "${EXT_DEPS}" "" "" "")

    PCL_ADD_INCLUDES("${SUBSYS_NAME}" "${SUBSYS_PATH}" ${incs})

    add_subdirectory(test)
    add_subdirectory(tools)
endif()


