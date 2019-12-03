// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#pragma once

#include <cuda.h>
#include <cuda_runtime.h>

#include "Open3D/Container/CudaUtils.cuh"
#include "Open3D/Container/Indexer.h"
#include "Open3D/Container/Kernel/Scheduler.h"
#include "Open3D/Container/SizeVector.h"
#include "Open3D/Container/Tensor.h"

// CUDA kernel launcher's goal is to separate scheduling (looping through each
// valid element) and computation (operations performed on each element).
//
// The kernel launch mechanism is inspired by PyTorch's launch Loops.cuh.
// See: https://tinyurl.com/y4lak257

static constexpr int64_t threads_per_block = 128;
static constexpr int64_t items_per_thread = 4;

namespace open3d {
namespace kernel {

// Applies f for each element
// Works for unary / binary elementwise operations
template <int64_t threads_per_block, int64_t items_per_thread, typename func_t>
__global__ void ElementWiseKernel(int64_t num_elems, func_t f) {
    int64_t items_per_block = threads_per_block * items_per_thread;
    int64_t idx = blockIdx.x * items_per_block + threadIdx.x;
#pragma unroll
    for (int64_t i = 0; i < items_per_thread; i++) {
        if (idx < num_elems) {
            f(idx);
            idx += threads_per_block;
        }
    }
}

class CUDALauncher {
public:
    template <typename scalar_t, typename func_t>
    static void LaunchUnaryEWKernel(const Tensor& src,
                                    Tensor& dst,
                                    func_t element_kernel) {
        OPEN3D_ASSERT_HOST_DEVICE_LAMBDA(func_t);
        Indexer indexer({src}, dst);

        int64_t num_elems = indexer.NumWorkloads();
        int64_t items_per_block = threads_per_block * items_per_thread;
        int64_t grid_size = (num_elems + items_per_block - 1) / items_per_block;

        auto f = [=] OPEN3D_HOST_DEVICE(int64_t workload_idx) {
            element_kernel(indexer.GetInputPtr(0, workload_idx),
                           indexer.GetOutputPtr(workload_idx));
        };

        ElementWiseKernel<threads_per_block, items_per_thread>
                <<<grid_size, threads_per_block, 0>>>(num_elems, f);
    }

    // dst = src[index_tensors]
    template <typename scalar_t, typename func_t>
    static void LaunchRhsIndexedUnaryEWKernel(
            const Tensor& src,
            Tensor& dst,
            const std::vector<Tensor>& index_tensors,
            const SizeVector& indexed_out_shape,
            func_t element_kernel) {
        OPEN3D_ASSERT_HOST_DEVICE_LAMBDA(func_t);

        std::vector<const int64_t*> index_tensor_data_ptrs;
        for (int64_t i = 0; i < index_tensors.size(); ++i) {
            auto index_tensor_ptr =
                    static_cast<const int64_t*>(index_tensors[i].GetDataPtr());
            index_tensor_data_ptrs.push_back(index_tensor_ptr);
        }

        std::vector<bool> is_trivial_dims;
        for (const Tensor& index_tensor : index_tensors) {
            is_trivial_dims.push_back(index_tensor.NumDims() == 0);
        }

        SizeVector mid_shape = indexed_out_shape;
        SizeVector mid_strides = Tensor::DefaultStrides(mid_shape);
        IndexedOffsetCalculator fancy_offset_calculator(
                src.GetShape(), src.GetStrides(), mid_strides, is_trivial_dims,
                index_tensor_data_ptrs);
        OffsetBroadcastCalculator broadcast_offset_calculator(
                mid_shape, mid_strides, dst.GetShape(),
                Tensor::DefaultStrides(dst.GetShape()));
        OffsetBroadcastCalculator dst_offset_calculator(
                dst.GetShape(), dst.GetStrides(), dst.GetShape(),
                Tensor::DefaultStrides(dst.GetShape()));

        int64_t num_elems = static_cast<int64_t>(dst.GetShape().NumElements());
        const char* src_data_ptr = static_cast<const char*>(src.GetDataPtr());
        char* dst_data_ptr = static_cast<char*>(dst.GetDataPtr());
        int64_t element_byte_size = DtypeUtil::ByteSize(src.GetDtype());

        auto f = [=] OPEN3D_HOST_DEVICE(int64_t thread_idx) {
            int64_t fancied_idx =
                    broadcast_offset_calculator.GetOffset(thread_idx);
            int64_t src_idx = fancy_offset_calculator.GetOffset(fancied_idx);
            const void* src_ptr = src_data_ptr + src_idx * element_byte_size;
            int64_t dst_idx = dst_offset_calculator.GetOffset(thread_idx);
            void* dst_ptr = dst_data_ptr + dst_idx * element_byte_size;
            element_kernel(src_ptr, dst_ptr);
        };

        int64_t items_per_block = threads_per_block * items_per_thread;
        int64_t grid_size = (num_elems + items_per_block - 1) / items_per_block;
        ElementWiseKernel<threads_per_block, items_per_thread>
                <<<grid_size, threads_per_block, 0>>>(num_elems, f);
    }

    // dst[index_tensors] = src
    template <typename scalar_t, typename func_t>
    static void LaunchLhsIndexedUnaryEWKernel(
            const Tensor& src,
            Tensor& dst,
            const std::vector<Tensor>& index_tensors,
            const SizeVector& indexed_out_shape,
            func_t element_kernel) {
        OPEN3D_ASSERT_HOST_DEVICE_LAMBDA(func_t);

        std::vector<const int64_t*> index_tensor_data_ptrs;
        for (int64_t i = 0; i < index_tensors.size(); ++i) {
            auto index_tensor_ptr =
                    static_cast<const int64_t*>(index_tensors[i].GetDataPtr());
            index_tensor_data_ptrs.push_back(index_tensor_ptr);
        }

        std::vector<bool> is_trivial_dims;
        for (const Tensor& index_tensor : index_tensors) {
            is_trivial_dims.push_back(index_tensor.NumDims() == 0);
        }

        // [src] --broadcast--> [mid] --fancy idx--> [dst]
        SizeVector mid_shape = indexed_out_shape;
        SizeVector mid_strides = Tensor::DefaultStrides(mid_shape);
        OffsetBroadcastCalculator src_offset_calculator(
                src.GetShape(), src.GetStrides(), mid_shape, mid_strides);
        IndexedOffsetCalculator dst_offset_calculator(
                dst.GetShape(), dst.GetStrides(), mid_strides, is_trivial_dims,
                index_tensor_data_ptrs);

        int64_t num_elems = static_cast<int64_t>(mid_shape.NumElements());
        const char* src_data_ptr = static_cast<const char*>(src.GetDataPtr());
        char* dst_data_ptr = static_cast<char*>(dst.GetDataPtr());
        int64_t element_byte_size = DtypeUtil::ByteSize(src.GetDtype());

        auto f = [=] OPEN3D_HOST_DEVICE(int64_t thread_idx) {
            int64_t src_idx = src_offset_calculator.GetOffset(thread_idx);
            const void* src_ptr = src_data_ptr + src_idx * element_byte_size;
            int64_t dst_idx = dst_offset_calculator.GetOffset(thread_idx);
            void* dst_ptr = dst_data_ptr + dst_idx * element_byte_size;
            element_kernel(src_ptr, dst_ptr);
        };

        int64_t items_per_block = threads_per_block * items_per_thread;
        int64_t grid_size = (num_elems + items_per_block - 1) / items_per_block;
        ElementWiseKernel<threads_per_block, items_per_thread>
                <<<grid_size, threads_per_block, 0>>>(num_elems, f);
    }
};

}  // namespace kernel
}  // namespace open3d
