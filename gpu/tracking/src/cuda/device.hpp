#ifndef PCL_GPU_TRACKING_DEVICE_HPP_
#define PCL_GPU_TRACKING_DEVICE_HPP_

#include "internal.h"

namespace pcl
{
	namespace device
	{
		__device__ __forceinline__ float
			getSampleNormal (const float mean, const float cov, murandState* rng_state)
		{
			float rn = murand_normal(rng_state);
			return (rn*cov + mean);
		}
	}
}

#endif // PCL_GPU_TRACKING_DEVICE_HPP_