set(SUBSYS_NAME gpu_people)
set(SUBSYS_PATH gpu/people)
set(SUBSYS_DESC "Point cloud people library (MUSA)")
set(SUBSYS_DEPS common features filters geometry gpu_containers gpu_utils io kdtree octree search segmentation surface visualization)

set(build FALSE)
PCL_SUBSYS_OPTION(build ${SUBSYS_NAME} ${SUBSYS_DESC} OFF)
PCL_SUBSYS_DEPEND(build ${SUBSYS_NAME} DEPS ${SUBSYS_DEPS})
mark_as_advanced("BUILD_${SUBSYS_NAME}")

PCL_ADD_DOC("${SUBSYS_NAME}")

if(build)
  REMOVE_VTK_DEFINITIONS()
  
  # MUSA NPP (mupp) libraries
  set(MUSA_LIB_DIR "/usr/local/musa/lib")
  
  FILE(GLOB srcs src/*.cpp src/*.h*)
  FILE(GLOB srcs_cuda src/cuda/*.cu src/cuda/*.h src/cuda/nvidia/*.cu src/cuda/nvidia/*.hpp)
  FILE(GLOB hdrs include/pcl/gpu/people/*.h*)
  FILE(GLOB hdrs_cuda src/cuda/nvidia/*.h*)

  source_group("Source files\\cuda" FILES ${srcs_cuda} )
  source_group("Source files" FILES ${srcs} )    

  # MUSA include directories
  include_directories(${VTK_INCLUDE_DIRS}
                      "${CMAKE_CURRENT_SOURCE_DIR}/include" 
                      "${CMAKE_CURRENT_SOURCE_DIR}/src" 
                      "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda" 
                      "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda/nvidia"
                      "${MUSA_LIB_DIR}/../include")
  include_directories(${OpenCV_INCLUDE_DIRS})

  if (UNIX OR APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  endif()
  if(NOT UNIX OR APPLE)
    add_definitions(-DPCLAPI_EXPORTS)
  endif()

  # Compile CUDA sources as object files
  set(MUSA_ARCH_FLAG "--offload-arch=mp_22")
  set_source_files_properties(${srcs_cuda} PROPERTIES 
      COMPILE_FLAGS "-mtgpu ${MUSA_ARCH_FLAG}"
      LANGUAGE CXX)
  
  # Create objects from CUDA sources
  set(objs_cuda)
  foreach(src ${srcs_cuda})
    get_filename_component(name ${src} NAME)
    set(obj_file "${CMAKE_CURRENT_BINARY_DIR}/${name}.o")
    add_custom_command(OUTPUT ${obj_file}
        COMMAND mcc -c ${src} -o ${obj_file} -mtgpu ${MUSA_ARCH_FLAG} -I/usr/local/musa/include -I"${CMAKE_CURRENT_SOURCE_DIR}/include" -I"${CMAKE_CURRENT_SOURCE_DIR}/src/cuda"
        DEPENDS ${src}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Compiling MUSA: ${src}")
    list(APPEND objs_cuda ${obj_file})
  endforeach()

  set(LIB_NAME "pcl_${SUBSYS_NAME}")

  PCL_ADD_LIBRARY("${LIB_NAME}" "${SUBSYS_NAME}" ${srcs} ${hdrs} ${srcs_cuda})
  target_link_libraries("${LIB_NAME}" 
      pcl_common pcl_io pcl_search pcl_surface pcl_segmentation pcl_features 
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
