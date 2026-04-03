#!/bin/bash
# PCL MUSA Build Script - All GPU Modules
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PCL_DIR="$SCRIPT_DIR"
BUILD_DIR="$PCL_DIR/build_musa"

echo "============================================"
echo "  PCL MUSA All GPU Modules Build Test"
echo "============================================"
echo "PCL Directory: $PCL_DIR"
echo "Build Directory: $BUILD_DIR"
echo ""

echo "=== Step 1: Prepare build directory ==="
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo ""
echo "=== Step 2: Configure CMake with ALL GPU modules ==="

cmake "$PCL_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_visualization=OFF \
    -DWITH_VTK=OFF \
    -DWITH_QT=OFF \
    -DBUILD_GPU=ON \
    -DBUILD_CUDA=OFF \
    -DBUILD_gpu_containers=ON \
    -DBUILD_gpu_utils=ON \
    -DBUILD_gpu_octree=ON \
    -DBUILD_gpu_features=ON \
    -DBUILD_gpu_segmentation=ON \
    -DBUILD_gpu_surface=ON \
    -DBUILD_gpu_tracking=ON \
    -DBUILD_gpu_kinfu=ON \
    -DBUILD_gpu_kinfu_large_scale=ON \
    -DBUILD_gpu_people=ON \
    -DBUILD_filters=ON \
    -DBUILD_surface=ON \
    -DBUILD_tools=OFF \
    -DBUILD_global_tests=ON \
    -DPCL_SHARED_LIBS=ON \
    -DBoost_USE_STATIC_LIBS=OFF \
    -DBOOST_ROOT=/usr \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_CXX_FLAGS="-Wno-conversion -Wno-unused-parameter -Wno-enum-compare -Wno-narrowing -DBOOST_BIND_GLOBAL_PLACEHOLDERS" \
    -DMUSA_FOUND=ON \
    -DMUSA_INCLUDE_DIR=/usr/local/musa/include \
    -DMUSA_LIBRARY_DIR=/usr/local/musa/lib \
    2>&1 | tail -60

if [ ! -f "$BUILD_DIR/Makefile" ]; then
    echo "ERROR: CMake configuration failed"
    exit 1
fi

echo ""
echo "=== Step 3: Compile ALL GPU modules ==="
make -j$(nproc) \
    pcl_gpu_containers \
    pcl_gpu_utils \
    pcl_gpu_octree \
    pcl_gpu_features \
    pcl_gpu_segmentation \
    pcl_gpu_surface \
    pcl_gpu_tracking \
    pcl_gpu_kinfu \
    pcl_gpu_kinfu_large_scale \
    pcl_gpu_people 2>&1 | tail -100

echo ""
echo "=== Step 4: Build verification ==="
LIB_COUNT=$(ls lib/libpcl_gpu*.so 2>/dev/null | wc -l)
echo "Built GPU Libraries: $LIB_COUNT"
ls -1 lib/libpcl_gpu*.so 2>/dev/null || echo "  None found"

echo ""
echo "============================================"
echo "  All GPU Modules Build Complete"
echo "============================================"
echo "GPU Libraries built: $LIB_COUNT"
echo ""
if [ "$LIB_COUNT" -ge 8 ]; then
    echo "Build Status: SUCCESS"
else
    echo "Build Status: PARTIAL"
fi
