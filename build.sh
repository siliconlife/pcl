#!/bin/bash
# PCL CUDA to MUSA Migration Build Script
# Use gcc for C++ code, mcc only for .cu files
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PCL_DIR="$SCRIPT_DIR"

BUILD_DIR="$PCL_DIR/build_musa"
MUSA_SDK="/usr/local/musa"
MUSA_ARCH="mp_22"

echo "============================================"
echo "  PCL MUSA GPU Build Script"
echo "============================================"
echo "PCL Directory: $PCL_DIR"
echo "Build Directory: $BUILD_DIR"
echo "MUSA SDK: $MUSA_SDK"
echo "Target Architecture: $MUSA_ARCH"
echo ""

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Create Boost endian.hpp stub
mkdir -p "$BUILD_DIR/boost_stubs/boost/detail"
mkdir -p "$BUILD_DIR/boost_stubs/cuda
cat > "$BUILD_DIR/boost_stubs/boost/detail/endian.hpp" << 'EOFHEADER'
#ifndef BOOST_DETAIL_ENDIAN_HPP
#define BOOST_DETAIL_ENDIAN_HPP
#include <cstdint>
#if defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) || defined(__AARCH64EL__)
  #define BOOST_ENDIAN_LITTLE_BYTE_ORDER 1
  #define BOOST_LITTLE_ENDIAN 1
#else
  #define BOOST_ENDIAN_LITTLE_BYTE_ORDER 1
  #define BOOST_LITTLE_ENDIAN 1
#endif
namespace boost { namespace detail {
enum endianness { little_endian_byte_order = 1, big_endian_byte_order = 2, native_endian_byte_order = 1 };
static const endianness host_byte_order = little_endian_byte_order;
} }
using boost::detail::little_endian_byte_order;
using boost::detail::big_endian_byte_order;
using boost::detail::native_endian_byte_order;
using boost::detail::host_byte_order;
using boost::detail::endianness;
#endif
EOFHEADER

echo "=== Creating compatibility headers ==="

# Create CUDA runtime API compatibility header
cat > "$BUILD_DIR/boost_stubs/cuda/cuda_runtime_api.h" << 'EOFCUDA'
#ifndef CUDA_RUNTIME_API_H
#define CUDA_RUNTIME_API_H
#include <musa_runtime_api.h>
#define cudaError_t musaError_t
#define cudaSuccess musaSuccess
#define cudaError musaError
#define cudaGetDevice musaGetDevice
#define cudaGetDeviceCount musaGetDeviceCount
#define cudaGetDeviceProperties musaGetDeviceProperties
#define cudaSetDevice musaSetDevice
#define cudaMalloc musaMalloc
#define cudaFree musaFree
#define cudaMemcpy musaMemcpy
#define cudaMemcpyHostToDevice musaMemcpyHostToDevice
#define cudaMemcpyDeviceToHost musaMemcpyDeviceToHost
#define cudaMemcpyKind musaMemcpyKind
#define cudaEventCreate musaEventCreate
#define cudaEventDestroy musaEventDestroy
#define cudaEventRecord musaEventRecord
#define cudaEventSynchronize musaEventSynchronize
#define cudaEventElapsedTime musaEventElapsedTime
#define cudaStreamCreate musaStreamCreate
#define cudaStreamDestroy musaStreamDestroy
#define cudaStreamSynchronize musaStreamSynchronize
#define cudaLaunch musaLaunch
#define cudaLaunchKernel musaLaunchKernel
#define cudaFuncSetCacheConfig musaFuncSetCacheConfig
#define cudaGetLastError musaGetLastError
#define cudaPeekAtLastError musaPeekAtLastError
#define cudaGetErrorString musaGetErrorString
#define cudaDeviceSynchronize musaDeviceSynchronize
#define cudaDriverGetVersion musaDriverGetVersion
#define cudaRuntimeGetVersion musaRuntimeGetVersion
#define cudaDeviceProp musaDeviceProp
#define cudaPointerAttributes musaPointerAttributes
#define cudaGetPointerAttributes musaGetPointerAttributes
#define cudaDeviceAttributeMultiprocessorCount musaDeviceAttributeMultiprocessorCount
#define cudaDeviceAttributeClockRate musaDeviceAttributeClockRate
#define cudaDeviceAttributeComputeCapabilityMajor musaDeviceAttributeComputeCapabilityMajor
#define cudaDeviceAttributeComputeCapabilityMinor musaDeviceAttributeComputeCapabilityMinor
#define cudaErrorMemoryAllocation musaErrorMemoryAllocation
#define cudaErrorLaunchOutOfResources musaErrorLaunchOutOfResources
#define cudaErrorLaunchFailure musaErrorLaunchFailure
#define cudaSuccess musaSuccess
#endif
EOFCUDA

echo ""

echo "=== Configuring with CMake (gcc for C++, mcc for .cu) ==="

# Use gcc for C++ code, build GPU modules
cmake "$PCL_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_CUDA=OFF \
    -DWITH_GPU=ON \
    -DBUILD_CUDA=ON \
    -DBUILD_GPU=ON \
    -DBUILD_common=ON \
    -DBUILD_io=ON \
    -DBUILD_kdtree=ON \
    -DBUILD_octree=ON \
    -DBUILD_search=ON \
    -DBUILD_sample_consensus=ON \
    -DBUILD_filters=OFF \
    -DBUILD_features=OFF \
    -DBUILD_segmentation=OFF \
    -DBUILD_surface=OFF \
    -DBUILD_registration=OFF \
    -DBUILD_recognition=OFF \
    -DBUILD_tools=OFF \
    -DBUILD_tests=OFF \
    -DPCL_SHARED_LIBS=ON \
    -DBoost_USE_STATIC_LIBS=OFF \
    -DBOOST_ROOT=/usr \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_CXX_FLAGS="-Wno-conversion -Wno-unused-parameter -Wno-enum-compare -DBOOST_LITTLE_ENDIAN=1 -I$BUILD_DIR/boost_stubs -I$MUSA_SDK/include -Wno-deprecated -DBOOST_BIND_GLOBAL_PLACEHOLDERS" \
    -DCMAKE_C_FLAGS="-I$BUILD_DIR/boost_stubs" \
    -DCMAKE_EXE_LINKER_FLAGS="-L$PCL_DIR/build_musa" \
    -DMUSA_FOUND=ON \
    -DMUSA_INCLUDE_DIR="$MUSA_SDK/include" \
    -DMUSA_LIBRARY_DIR="$MUSA_SDK/lib64" \
    2>&1 | tee cmake_output.log || true

if [ -f "$BUILD_DIR/Makefile" ]; then
    echo ""
    echo "=== Compiling PCL with MUSA GPU ==="
    make -j$(nproc) 2>&1 | tee compile_output.log || true
else
    echo ""
    echo "CMake configuration incomplete. Check cmake_output.log"
    tail -50 cmake_output.log
fi

echo ""
echo "=== Build Complete ==="
echo ""
echo "Built Libraries:"
ls -la "$BUILD_DIR/lib/"*.so 2>/dev/null | head -20 || echo "No shared libraries found"
echo ""
echo "Built Executables:"
ls -la "$BUILD_DIR/bin/" 2>/dev/null | head -10 || echo "No executables found"