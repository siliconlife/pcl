#ifndef PCL_GPU_OCTREE_CUDA_INTERNAL_HPP_
#define PCL_GPU_OCTREE_CUDA_INTERNAL_HPP_

#include <vector>
#include <pcl/gpu/containers/device_array.h>
#include <pcl/gpu/octree/device_format.hpp>
#include <pcl/gpu/utils/safe_call.hpp>

#include "musa.h"

namespace pcl
{
    namespace gpu
    {
        typedef DeviceArray<float4> PointCloud;
    }

    namespace device
    {
        struct OctreeGlobal
        {
            int* nodes;
            int* codes;
            int* begs;
            int* ends;
            int* nodes_num;
            int* parent;

            OctreeGlobal() : nodes(0), codes(0), begs(0), ends(0), nodes_num(0), parent(0) {}
        };

        struct OctreeGlobalWithBox : public OctreeGlobal
        {
            float3 minp, maxp;
        };

        class OctreeImpl
        {
        public:
            typedef float4 PointType;
            typedef DeviceArray<PointType> PointArray;
            typedef PointArray PointCloud;
            typedef PointArray Queries;
            typedef DeviceArray<float> Radiuses;
            typedef DeviceArray<int> BatchResult;
            typedef DeviceArray<int> BatchResultSizes;
            typedef DeviceArray<float> BatchResultSqrDists;
            typedef DeviceArray<int> Indices;
            typedef pcl::gpu::NeighborIndices NeighborIndices;

            static void get_gpu_arch_compiled_for(int& bin, int& ptr);

            OctreeImpl() {}
            ~OctreeImpl() {}

            void setCloud(const PointCloud& input_points);
            void build();
            void radiusSearchHost(const PointType& center, float radius, std::vector<int>& out, int max_nn) const;
            void approxNearestSearchHost(const PointType& query, int& out_index, float& sqr_dist) const;

            void radiusSearch(const Queries& queries, float radius, NeighborIndices& results);
            void radiusSearch(const Queries& queries, const Radiuses& radiuses, NeighborIndices& results);
            void radiusSearch(const Queries& queries, const Indices& indices, float radius, NeighborIndices& results);
            void approxNearestSearch(const Queries& queries, NeighborIndices& results) const;
            void nearestKSearchBatch(const Queries& queries, int k, NeighborIndices& results) const;

            PointCloud points;
            DeviceArray2D<float> points_sorted;
            DeviceArray<int> codes;
            DeviceArray<int> indices;
            OctreeGlobalWithBox octreeGlobal;
            DeviceArray2D<int> storage;

            struct OctreeDataHost
            {
                std::vector<int> nodes;
                std::vector<int> codes;
                std::vector<int> begs;
                std::vector<int> ends;
                std::vector<int> indices;
                std::vector<float> points_sorted;
                int points_sorted_step;
                int downloaded;
            } host_octree;

            void internalDownload();

        private:
            template<typename BatchType>
            void radiusSearchEx(BatchType& batch, const Queries& queries, NeighborIndices& results);
        };

        void bruteForceRadiusSearch(const OctreeImpl::PointCloud& cloud, const OctreeImpl::PointType& query, float radius, DeviceArray<int>& result, DeviceArray<int>& buffer);
    }
}

#endif
