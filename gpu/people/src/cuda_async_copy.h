/*
 * Software License Agreement (BSD License)
 *
 *  Point Cloud Library (PCL) - www.pointclouds.org
 *  Copyright (c) 2011, Willow Garage, Inc.
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * @authors: Anatoly Baksheev
 */


#ifndef PCL_GPU_PEOPLE_CUDA_ASYNC_COPY_H_
#define PCL_GPU_PEOPLE_CUDA_ASYNC_COPY_H_

#include <pcl/gpu/containers/device_array.h>
#include <pcl/gpu/utils/safe_call.hpp>

namespace pcl
{
  namespace gpu
  {
    template<class T>
    class AsyncCopy
    {
    public:
      AsyncCopy(T* ptr, size_t size) : ptr_(ptr)
      {
        cudaSafeCall( musaHostRegister(ptr_, size, 0) );        
        cudaSafeCall( musaStreamCreate(&stream_) );
      }

      AsyncCopy(std::vector<T>& data) : ptr_(&data[0])
      {
        cudaSafeCall( musaHostRegister(ptr_, data.size(), 0) );        
        cudaSafeCall( musaStreamCreate(&stream_) );
      }

      ~AsyncCopy()
      {          
        cudaSafeCall( musaHostUnregister(ptr_) );
        cudaSafeCall( musaStreamDestroy(stream_) );
      }

      void download(const DeviceArray<T>& arr)
      {
        cudaSafeCall( musaMemcpyAsync(ptr_, arr.ptr(), arr.sizeBytes(), musaMemcpyDeviceToHost, stream_) );	
      }

      void download(const DeviceArray2D<T>& arr)
      {
        cudaSafeCall( musaMemcpy2DAsync(ptr_, arr.cols(), arr.ptr(), arr.step(), arr.colsBytes(), arr.rows(), musaMemcpyDeviceToHost, stream_) );
      }

      void upload(const DeviceArray<T>& arr) const 
      {
          cudaSafeCall( musaMemcpyAsync(arr.ptr(), ptr_, arr.size(), musaMemcpyHostToDevice, stream_) );	
      }

      void upload(const DeviceArray2D<T>& arr) const 
      {
        cudaSafeCall( musaMemcpy2DAsync(arr.ptr(), arr.step(), ptr_, arr.cols(), arr.colsBytes(), arr.rows(), musaMemcpyHostToDevice, stream_) );
      }

      void waitForCompeltion()
      {
        cudaSafeCall( musaStreamSynchronize(stream_) );
      }
    private:
      musaStream_t stream_;
      T* ptr_   ;
    };
  }

  namespace device
  {
    using pcl::gpu::AsyncCopy;
  }
}

#endif /* PCL_GPU_PEOPLE_CUDA_ASYNC_COPY_H_ */

