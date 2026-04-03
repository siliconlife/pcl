#!/bin/bash
# PCL CUDA to MUSA Migration Build Script - Unified CPU & GPU Build
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PCL_DIR="$SCRIPT_DIR"
BUILD_DIR="$PCL_DIR/build_musa"

# Default build mode: all (both CPU and GPU)
BUILD_MODE="${1:-all}"

# Function to display usage
usage() {
    echo "Usage: $0 [mode]"
    echo ""
    echo "Modes:"
    echo "  cpu   - Build CPU modules only"
    echo "  all   - Build both CPU and GPU modules (default)"
    echo ""
    echo "Examples:"
    echo "  $0 cpu    # Build only CPU libraries"
    echo "  $0 all    # Build everything"
    echo "  $0        # Same as 'all'"
}

# Validate build mode
case "$BUILD_MODE" in
    cpu|all)
        ;;
    -h|--help|help)
        usage
        exit 0
        ;;
    *)
        echo "Error: Unknown build mode '$BUILD_MODE'"
        usage
        exit 1
        ;;
esac

echo "============================================"
echo "  PCL MUSA Unified Build Script"
echo "============================================"
echo "Build Mode: $BUILD_MODE"
echo "PCL Directory: $PCL_DIR"
echo "Build Directory: $BUILD_DIR"
echo ""

# Step 1: Install and Build GTest
echo "=== Step 1: Install and Build GTest ==="

if [ ! -f /usr/lib/libgtest.so ]; then
    echo "  Installing libgtest-dev..."
    sudo apt-get update -qq 2>/dev/null || true
    sudo apt-get install -y -qq libgtest-dev cmake 2>/dev/null || true
    
    GTEST_SRC=""
    if [ -d "/usr/src/gtest" ]; then
        GTEST_SRC="/usr/src/gtest"
    elif [ -d "/usr/src/googletest/googletest" ]; then
        GTEST_SRC="/usr/src/googletest/googletest"
    fi
    
    if [ -n "$GTEST_SRC" ] && [ -f "$GTEST_SRC/CMakeLists.txt" ]; then
        echo "  Building GTest shared library..."
        cd "$GTEST_SRC"
        sudo cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=/usr .
        sudo make -j$(nproc)
        sudo cp lib/libgtest.so lib/libgtest_main.so /usr/lib/ 2>/dev/null || true
        sudo ldconfig
        echo "  GTest shared library built"
    fi
fi

if [ -f /usr/lib/libgtest.so ] || [ -f /usr/lib/libgtest.a ]; then
    echo "  GTest ready: $(ls /usr/lib/libgtest* 2>/dev/null | tr '\n' ' ')"
else
    echo "  Warning: GTest not found"
fi
echo ""

# Step 2: Prepare build directory
echo "=== Step 2: Prepare build directory ==="
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
echo ""

# Step 3: Configure CMake based on build mode
echo "=== Step 3: Configure CMake ==="

# Base CMake arguments
CMAKE_ARGS=(
    "$PCL_DIR"
    -DCMAKE_BUILD_TYPE=Release
    -DBUILD_visualization=OFF
    -DWITH_VTK=OFF
    -DWITH_QT=OFF
    -DBUILD_GPU=ON
    -DBUILD_CUDA=OFF
    -DPCL_SHARED_LIBS=ON
    -DBoost_USE_STATIC_LIBS=OFF
    -DBOOST_ROOT=/usr
    -DCMAKE_C_COMPILER=gcc
    -DCMAKE_CXX_COMPILER=g++
    -DCMAKE_CXX_FLAGS="-Wno-conversion -Wno-unused-parameter -Wno-enum-compare -Wno-narrowing -DBOOST_BIND_GLOBAL_PLACEHOLDERS"
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    -Wno-dev
    -DMUSA_FOUND=ON
    -DMUSA_INCLUDE_DIR=/usr/local/musa/include
    -DMUSA_LIBRARY_DIR=/usr/local/musa/lib
)

# Add mode-specific configuration
case "$BUILD_MODE" in
    cpu)
        echo "Configuring for CPU modules only..."
        CMAKE_ARGS+=(
            -DBUILD_gpu_containers=OFF
            -DBUILD_gpu_utils=OFF
            -DBUILD_gpu_octree=OFF
            -DBUILD_gpu_features=OFF
            -DBUILD_gpu_segmentation=OFF
            -DBUILD_gpu_surface=OFF
            -DBUILD_gpu_tracking=OFF
            -DBUILD_gpu_kinfu=OFF
            -DBUILD_gpu_kinfu_large_scale=OFF
            -DBUILD_gpu_people=OFF
            -DBUILD_filters=OFF
            -DBUILD_surface=OFF
            -DBUILD_tools=OFF
            -DBUILD_global_tests=ON
            -DGTEST_ROOT=/usr/src/gtest
        )
        ;;
    all)
        echo "Configuring for all modules (CPU + GPU)..."
        CMAKE_ARGS+=(
            -DBUILD_gpu_containers=ON
            -DBUILD_gpu_utils=ON
            -DBUILD_gpu_octree=ON
            -DBUILD_gpu_features=ON
            -DBUILD_gpu_segmentation=ON
            -DBUILD_gpu_surface=ON
            -DBUILD_gpu_tracking=ON
            -DBUILD_gpu_kinfu=OFF
            -DBUILD_gpu_kinfu_large_scale=OFF
            -DBUILD_gpu_people=OFF
            -DBUILD_filters=ON
            -DBUILD_surface=ON
            -DBUILD_tools=OFF
            -DBUILD_global_tests=ON
            -DGTEST_ROOT=/usr/src/gtest
        )
        ;;
esac

cmake "${CMAKE_ARGS[@]}" 2>&1 | tail -50

if [ ! -f "$BUILD_DIR/Makefile" ]; then
    echo "ERROR: CMake configuration failed"
    exit 1
fi

echo ""

# Step 4: Compile based on build mode
echo "=== Step 4: Compile ==="

case "$BUILD_MODE" in
    cpu)
        echo "Building CPU modules..."
        make -j$(nproc) 2>&1 | tail -40
        ;;
    all)
        echo "Building all modules (CPU + GPU)..."
        # First build GPU modules to ensure they're available
        make -j$(nproc) pcl_gpu_containers pcl_gpu_utils pcl_gpu_octree pcl_gpu_features pcl_gpu_segmentation pcl_gpu_surface pcl_gpu_tracking pcl_filters pcl_surface 2>&1 | tail -30
        # Then build everything else including tests
        make -j$(nproc) tests 2>&1 | tail -40
        ;;
esac

echo ""

# Step 5: Build verification
echo "=== Step 5: Build verification ==="

CPU_LIB_COUNT=$(ls lib/libpcl_*.so 2>/dev/null | grep -v libpcl_gpu | wc -l)
GPU_LIB_COUNT=$(ls lib/libpcl_gpu*.so 2>/dev/null | wc -l)

echo "Built CPU Libraries: $CPU_LIB_COUNT"
echo "Built GPU Libraries: $GPU_LIB_COUNT"

echo ""
echo "CPU Libraries:"
ls -1 lib/libpcl_*.so 2>/dev/null | grep -v libpcl_gpu || echo "  None found"

echo ""
echo "GPU Libraries:"
ls -1 lib/libpcl_gpu*.so 2>/dev/null || echo "  None found"

# Step 6: Run tests using ctest
echo ""
echo "=== Step 6: Run tests ==="
cd "$BUILD_DIR"

export LD_LIBRARY_PATH="$BUILD_DIR/lib:$LD_LIBRARY_PATH"

# Run all tests using ctest
echo "Running ctest..."
ctest --output-on-failure 2>&1 | tail -50

# Count test results
echo ""
echo "=== Test Summary ==="
TEST_COUNT=$(find "$BUILD_DIR/test" -type f -executable -name "test_*" 2>/dev/null | wc -l)
echo "Test binaries built: $TEST_COUNT"

echo ""
echo "============================================"
echo "  Build Complete"
echo "============================================"
echo "Build Mode: $BUILD_MODE"
echo "CPU Libraries built: $CPU_LIB_COUNT"
echo "GPU Libraries built: $GPU_LIB_COUNT"
echo ""

# Determine success/failure
if [ "$BUILD_MODE" = "cpu" ]; then
    if [ "$CPU_LIB_COUNT" -gt 0 ]; then
        echo "Build Status: SUCCESS"
    else
        echo "Build Status: FAILED"
        exit 1
    fi
else
    # all mode
    if [ "$CPU_LIB_COUNT" -gt 0 ] && [ "$GPU_LIB_COUNT" -gt 0 ]; then
        echo "Build Status: SUCCESS"
    elif [ "$CPU_LIB_COUNT" -gt 0 ] || [ "$GPU_LIB_COUNT" -gt 0 ]; then
        echo "Build Status: PARTIAL SUCCESS"
    else
        echo "Build Status: FAILED"
        exit 1
    fi
fi

echo ""
echo "To run tests manually: cd build_musa && ctest --output-on-failure"
echo ""
echo "Usage reminder:"
echo "  $0 cpu  # Build CPU only"
echo "  $0 all  # Build everything (default)"
