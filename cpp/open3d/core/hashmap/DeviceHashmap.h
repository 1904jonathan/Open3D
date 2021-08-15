// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018-2021 www.open3d.org
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

#include "open3d/core/CUDAUtils.h"
#include "open3d/core/MemoryManager.h"
#include "open3d/core/Tensor.h"
#include "open3d/core/hashmap/HashmapBuffer.h"

namespace open3d {
namespace core {

enum class HashmapBackend;

class DeviceHashmap {
public:
    DeviceHashmap(int64_t init_capacity,
                  int64_t dsize_key,
                  std::vector<int64_t> dsize_values,
                  const Device& device)
        : capacity_(init_capacity),
          dsize_key_(dsize_key),
          dsize_values_(dsize_values),
          device_(device) {}
    virtual ~DeviceHashmap() {}

    /// Rehash expects a lot of extra memory space at runtime,
    /// since it consists of
    /// 1) dumping all key value pairs to a buffer
    /// 2) creating a new hash table
    /// 3) parallel inserting dumped key value pairs
    /// 4) deallocating old hash table
    virtual void Rehash(int64_t buckets) = 0;

    /// Parallel insert contiguous arrays of keys and values.
    virtual void Insert(const void* input_keys,
                        const std::vector<const void*> input_values,
                        buf_index_t* output_buf_indices,
                        bool* output_masks,
                        int64_t count) = 0;

    /// Parallel activate contiguous arrays of keys without copying values.
    /// Specifically useful for large value elements (e.g., a tensor), where we
    /// can do in-place management after activation.
    virtual void Activate(const void* input_keys,
                          buf_index_t* output_buf_indices,
                          bool* output_masks,
                          int64_t count) = 0;

    /// Parallel find a contiguous array of keys.
    virtual void Find(const void* input_keys,
                      buf_index_t* output_buf_indices,
                      bool* output_masks,
                      int64_t count) = 0;

    /// Parallel erase a contiguous array of keys.
    virtual void Erase(const void* input_keys,
                       bool* output_masks,
                       int64_t count) = 0;

    /// Parallel collect all iterators in the hash table
    virtual int64_t GetActiveIndices(buf_index_t* output_buf_indices) = 0;

    /// Clear stored map without reallocating memory.
    virtual void Clear() = 0;

    /// Get the size (number of valid entries) of the hash map.
    virtual int64_t Size() const = 0;

    /// Get the number of buckets of the hash map.
    virtual int64_t GetBucketCount() const = 0;

    /// Get the current load factor, defined as size / bucket count.
    virtual float LoadFactor() const = 0;

    /// Get the maximum capacity of the hash map.
    int64_t GetCapacity() const { return capacity_; }

    /// Get the current device.
    Device GetDevice() const { return device_; }

    /// Get the number of entries per bucket.
    virtual std::vector<int64_t> BucketSizes() const = 0;

    /// Get the key buffer that stores actual keys.
    Tensor GetKeyBuffer() { return buffer_->GetKeyBuffer(); }

    /// Get the value buffers that store actual array of values.
    std::vector<Tensor> GetValueBuffers() { return buffer_->GetValueBuffers(); }

    /// Get the i-th value buffer that store an actual value array.
    Tensor GetValueBuffer(size_t i = 0) { return buffer_->GetValueBuffer(i); }

public:
    int64_t capacity_;

    int64_t dsize_key_;
    std::vector<int64_t> dsize_values_;

    Device device_;

    std::shared_ptr<HashmapBuffer> buffer_;
};

/// Factory functions:
/// - Default constructor switch is in DeviceHashmap.cpp
/// - Default CPU constructor is in CPU/CreateCPUHashmap.cpp
/// - Default CUDA constructor is in CUDA/CreateCUDAHashmap.cu
std::shared_ptr<DeviceHashmap> CreateDeviceHashmap(
        int64_t init_capacity,
        const Dtype& dtype_key,
        const SizeVector& element_shape_key,
        const std::vector<Dtype>& dtype_values,
        const std::vector<SizeVector>& element_shape_values,
        const Device& device,
        const HashmapBackend& backend);

std::shared_ptr<DeviceHashmap> CreateCPUHashmap(
        int64_t init_capacity,
        const Dtype& dtype_key,
        const SizeVector& element_shape_key,
        const std::vector<Dtype>& dtype_values,
        const std::vector<SizeVector>& element_shape_values,
        const Device& device,
        const HashmapBackend& backend);

std::shared_ptr<DeviceHashmap> CreateCUDAHashmap(
        int64_t init_capacity,
        const Dtype& dtype_key,
        const SizeVector& element_shape_key,
        const std::vector<Dtype>& dtype_values,
        const std::vector<SizeVector>& element_shape_values,
        const Device& device,
        const HashmapBackend& backend);

}  // namespace core
}  // namespace open3d
