set(SUBSYS_NAME gpu_people)
set(SUBSYS_PATH gpu/people)
set(SUBSYS_DESC "Point cloud people library (MUSA)")
set(SUBSYS_DEPS common features filters geometry gpu_containers gpu_utils io kdtree octree search segmentation)

set(build FALSE)
PCL_SUBSYS_OPTION(build ${SUBSYS_NAME} ${SUBSYS_DESC} ON)
PCL_SUBSYS_DEPEND(build ${SUBSYS_NAME} DEPS ${SUBSYS_DEPS})
mark_as_advanced("BUILD_${SUBSYS_NAME}")

PCL_ADD_DOC("${SUBSYS_NAME}")

if(build)
  # MUSA NPP (mupp) libraries
  set(MUSA_LIB_DIR "/usr/local/musa/lib")
  
  FILE(GLOB srcs src/*.cpp src/*.h*)
  FILE(GLOB srcs_cuda src/cuda/*.cu src/cuda/*.h src/cuda/nvidia/*.cu src/cuda/nvidia/*.hpp)
  FILE(GLOB hdrs include/pcl/gpu/people/*.h*)
  FILE(GLOB hdrs_cuda src/cuda/nvidia/*.h*)

  source_group("Source files\\cuda" FILES ${srcs_cuda} )
  source_group("Source files" FILES ${srcs} )    

  # MUSA include directories
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include" 
                      "${CMAKE_CURRENT_SOURCE_DIR}/src" 
                      "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda" 
                      "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda/nvidia"
                      "${MUSA_LIB_DIR}/../include")

  if (UNIX OR APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  endif()
  if(NOT UNIX OR APPLE)
    add_definitions(-DPCLAPI_EXPORTS)
  endif()

  set(LIB_NAME "pcl_${SUBSYS_NAME}")

  PCL_CUDA_ADD_LIBRARY("${LIB_NAME}" "${SUBSYS_NAME}" ${srcs} ${hdrs} ${srcs_cuda})
  target_link_libraries("${LIB_NAME}" 
      pcl_common pcl_io pcl_search pcl_segmentation pcl_features 
      pcl_sample_consensus pcl_gpu_utils pcl_gpu_containers 
      ${MUSA_LIB_DIR}/libmusart.so
      ${MUSA_LIB_DIR}/libmuppial.so
      ${MUSA_LIB_DIR}/libmuppicc.so
      ${MUSA_LIB_DIR}/libmuppist.so
  )
  target_link_directories("${LIB_NAME}" PRIVATE ${MUSA_LIB_DIR})
  
  PCL_MAKE_PKGCONFIG("${LIB_NAME}" "${SUBSYS_NAME}" "${SUBSYS_DESC}" "${SUBSYS_DEPS}" "" "" "" "")

  # install include files
  PCL_ADD_INCLUDES("${SUBSYS_NAME}" "${SUBSYS_PATH}" ${hdrs} ${hdrs_cuda})

  add_subdirectory(tools)
endif(build)
