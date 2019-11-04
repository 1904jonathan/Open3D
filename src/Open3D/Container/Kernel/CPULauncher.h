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
#include <vector>

#include "Open3D/Container/Tensor.h"

namespace open3d {
namespace kernel {

class CPULauncher {
public:
    /// Recover src tensor element offsets given dst tensor element offsets
    /// src and dst tensors have the same size but different strides
    class OffsetCalculator {
    public:
        OffsetCalculator(const std::vector<size_t>& src_strides,
                         const std::vector<size_t>& dst_strides)
            : num_dims_(src_strides.size()),
              src_strides_(src_strides),
              dst_strides_(dst_strides) {}

        size_t GetOffset(size_t dst_idx) const {
            size_t src_idx = 0;
            for (size_t dim = 0; dim < num_dims_; dim++) {
                src_idx += dst_idx / dst_strides_[dim] * src_strides_[dim];
                dst_idx = dst_idx % dst_strides_[dim];
            }
            return src_idx;
        }

    protected:
        size_t num_dims_;
        std::vector<size_t> src_strides_;
        std::vector<size_t> dst_strides_;
    };

    class IndexedOffsetCalculator {
    public:
        IndexedOffsetCalculator(
                const std::vector<size_t>& src_strides,
                const std::vector<size_t>& dst_strides,
                const std::vector<size_t>& indexing_shapes,
                const std::vector<const int*>& indexing_tensor_data_ptrs)
            : num_dims_(src_strides.size()),
              src_strides_(src_strides),
              dst_strides_(dst_strides),
              indexing_shapes_(indexing_shapes),
              indexing_tensor_data_ptrs_(indexing_tensor_data_ptrs) {}

        size_t GetOffset(size_t thread_idx) const {
            size_t output_idx = 0;
            for (size_t dim = 0; dim < num_dims_; dim++) {
                size_t dim_idx = thread_idx / dst_strides_[dim];
                size_t dim_size = indexing_shapes_[dim];

                if (dim_size == 0) {
                    output_idx += dim_idx * src_strides_[dim];
                } else if (dim_size == 1) {
                    // TODO assert, negative indexing; reduce dimension
                    output_idx += indexing_tensor_data_ptrs_[dim][0] *
                                  src_strides_[dim];
                } else {
                    // TODO assert, negative indexing
                    output_idx += indexing_tensor_data_ptrs_[dim][dim_idx] *
                                  src_strides_[dim];
                }
                thread_idx = thread_idx % dst_strides_[dim];
            }
            return output_idx;
        }

    protected:
        size_t num_dims_;
        std::vector<size_t> src_strides_;
        std::vector<size_t> dst_strides_;
        std::vector<size_t> indexing_shapes_;
        std::vector<const int*> indexing_tensor_data_ptrs_;
    };

public:
    template <typename scalar_t, typename func_t>
    static void LaunchUnaryEWKernel(const Tensor& src,
                                    Tensor& dst,
                                    func_t element_kernel) {
        const char* src_data_ptr = static_cast<const char*>(src.GetDataPtr());
        char* dst_data_ptr = static_cast<char*>(dst.GetDataPtr());
        size_t element_byte_size = DtypeUtil::ByteSize(src.GetDtype());
        OffsetCalculator offset_calculator(src.GetStrides(), dst.GetStrides());

#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int64_t dst_idx = 0;
             dst_idx < static_cast<int64_t>(src.GetShape().NumElements());
             dst_idx++) {
            size_t src_idx = offset_calculator.GetOffset(dst_idx);
            const void* src_ptr = src_data_ptr + src_idx * element_byte_size;
            void* dst_ptr = dst_data_ptr + dst_idx * element_byte_size;
            element_kernel(src_ptr, dst_ptr);
        }
    }

    template <typename scalar_t, typename func_t>
    static void LaunchIndexedUnaryEWKernel(const Tensor& src,
                                           Tensor& dst,
                                           const std::vector<Tensor>& indices,
                                           const SizeVector& indexing_shapes,
                                           func_t element_kernel) {
        utility::LogInfo("IndexedKernel!");

        std::vector<const int*> indexing_tensor_data_ptrs;
        utility::LogInfo("src.GetStrides() = {}", src.GetStrides());
        utility::LogInfo("dst.GetStrides() = {}", dst.GetStrides());
        for (int i = 0; i < indices.size(); ++i) {
            utility::LogInfo("indices[{}] {}", i, indices[i].ToString());
            auto index_tensor_ptr =
                    static_cast<const int*>(indices[i].GetDataPtr());

            std::vector<int> tmp;
            for (auto j = 0; j < indexing_shapes[i]; ++j) {
                tmp.push_back(index_tensor_ptr[j]);
            }
            utility::LogInfo("index_tensor_ptr: {}", tmp);

            indexing_tensor_data_ptrs.push_back(index_tensor_ptr);
        }

        IndexedOffsetCalculator src_offset_calculator(
                src.GetStrides(), dst.GetStrides(), indexing_shapes,
                indexing_tensor_data_ptrs);

        int64_t num_elems = static_cast<int64_t>(dst.GetShape().NumElements());
        const char* src_data_ptr = static_cast<const char*>(src.GetDataPtr());
        char* dst_data_ptr = static_cast<char*>(dst.GetDataPtr());
        size_t element_byte_size = DtypeUtil::ByteSize(src.GetDtype());

#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (int64_t dst_idx = 0; dst_idx < num_elems; dst_idx++) {
            size_t src_idx = src_offset_calculator.GetOffset(dst_idx);
            const void* src_ptr = src_data_ptr + src_idx * element_byte_size;
            void* dst_ptr = dst_data_ptr + dst_idx * element_byte_size;
            element_kernel(src_ptr, dst_ptr);
        }
    }
};

}  // namespace kernel
}  // namespace open3d
