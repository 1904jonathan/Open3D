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

#include "open3d/core/hashmap/Hashmap.h"

#include "open3d/core/Tensor.h"
#include "open3d/core/hashmap/DeviceHashBackend.h"
#include "open3d/t/io/HashmapIO.h"
#include "open3d/utility/Helper.h"
#include "open3d/utility/Logging.h"

namespace open3d {
namespace core {

Hashmap::Hashmap(int64_t init_capacity,
                 const Dtype& key_dtype,
                 const SizeVector& key_element_shape,
                 const Dtype& value_dtype,
                 const SizeVector& value_element_shape,
                 const Device& device,
                 const HashBackendType& backend)
    : key_dtype_(key_dtype),
      key_element_shape_(key_element_shape),
      dtypes_value_({value_dtype}),
      element_shapes_value_({value_element_shape}) {
    Init(init_capacity, device, backend);
}

Hashmap::Hashmap(int64_t init_capacity,
                 const Dtype& key_dtype,
                 const SizeVector& key_element_shape,
                 const std::vector<Dtype>& dtypes_value,
                 const std::vector<SizeVector>& element_shapes_value,
                 const Device& device,
                 const HashBackendType& backend)
    : key_dtype_(key_dtype),
      key_element_shape_(key_element_shape),
      dtypes_value_(dtypes_value),
      element_shapes_value_(element_shapes_value) {
    Init(init_capacity, device, backend);
}

void Hashmap::Rehash(int64_t buckets) {
    return device_hashmap_->Rehash(buckets);
}

void Hashmap::InsertImpl(const Tensor& input_keys,
                         const std::vector<Tensor>& input_values_soa,
                         Tensor& output_buf_indices,
                         Tensor& output_masks) {
    CheckKeyValueLengthCompatibility(input_keys, input_values_soa);
    CheckKeyCompatibility(input_keys);
    CheckValueCompatibility(input_values_soa);

    int64_t length = input_keys.GetLength();
    PrepareIndicesOutput(output_buf_indices, length);
    PrepareMasksOutput(output_masks, length);

    std::vector<const void*> input_values_ptrs;
    for (auto& input_value : input_values_soa) {
        input_values_ptrs.push_back(input_value.GetDataPtr());
    }

    device_hashmap_->Insert(
            input_keys.GetDataPtr(), input_values_ptrs,
            static_cast<buf_index_t*>(output_buf_indices.GetDataPtr()),
            output_masks.GetDataPtr<bool>(), length);
}

void Hashmap::Insert(const Tensor& input_keys,
                     const Tensor& input_values,
                     Tensor& output_buf_indices,
                     Tensor& output_masks) {
    InsertImpl(input_keys, {input_values}, output_buf_indices, output_masks);
}

void Hashmap::Insert(const Tensor& input_keys,
                     const std::vector<Tensor>& input_values_soa,
                     Tensor& output_buf_indices,
                     Tensor& output_masks) {
    InsertImpl(input_keys, input_values_soa, output_buf_indices, output_masks);
}

void Hashmap::Activate(const Tensor& input_keys,
                       Tensor& output_buf_indices,
                       Tensor& output_masks) {
    CheckKeyLength(input_keys);
    CheckKeyCompatibility(input_keys);

    int64_t length = input_keys.GetLength();
    PrepareIndicesOutput(output_buf_indices, length);
    PrepareMasksOutput(output_masks, length);

    device_hashmap_->Activate(
            input_keys.GetDataPtr(),
            static_cast<buf_index_t*>(output_buf_indices.GetDataPtr()),
            output_masks.GetDataPtr<bool>(), length);
}

void Hashmap::Find(const Tensor& input_keys,
                   Tensor& output_buf_indices,
                   Tensor& output_masks) {
    CheckKeyLength(input_keys);
    CheckKeyCompatibility(input_keys);

    int64_t length = input_keys.GetLength();
    PrepareIndicesOutput(output_buf_indices, length);
    PrepareMasksOutput(output_masks, length);

    device_hashmap_->Find(
            input_keys.GetDataPtr(),
            static_cast<buf_index_t*>(output_buf_indices.GetDataPtr()),
            output_masks.GetDataPtr<bool>(), length);
}

void Hashmap::Erase(const Tensor& input_keys, Tensor& output_masks) {
    CheckKeyLength(input_keys);
    CheckKeyCompatibility(input_keys);

    int64_t length = input_keys.GetLength();
    PrepareMasksOutput(output_masks, length);

    device_hashmap_->Erase(input_keys.GetDataPtr(),
                           output_masks.GetDataPtr<bool>(), length);
}

void Hashmap::GetActiveIndices(Tensor& output_buf_indices) const {
    int64_t length = device_hashmap_->Size();
    PrepareIndicesOutput(output_buf_indices, length);

    device_hashmap_->GetActiveIndices(
            static_cast<buf_index_t*>(output_buf_indices.GetDataPtr()));
}

void Hashmap::Clear() { device_hashmap_->Clear(); }

void Hashmap::Save(const std::string& file_name) {
    t::io::WriteHashmap(file_name, *this);
}

Hashmap Hashmap::Load(const std::string& file_name) {
    return t::io::ReadHashmap(file_name);
}

Hashmap Hashmap::Clone() const { return To(GetDevice(), /*copy=*/true); }

Hashmap Hashmap::To(const Device& device, bool copy) const {
    if (!copy && GetDevice() == device) {
        return *this;
    }

    Tensor keys = GetKeyTensor();
    std::vector<Tensor> values = GetValueTensors();

    Tensor active_buf_indices_i32;
    GetActiveIndices(active_buf_indices_i32);
    Tensor active_indices = active_buf_indices_i32.To(core::Int64);

    Tensor active_keys = keys.IndexGet({active_indices}).To(device);
    std::vector<Tensor> soa_active_values;
    for (auto& value : values) {
        soa_active_values.push_back(
                value.IndexGet({active_indices}).To(device));
    }

    Hashmap new_hashmap(GetCapacity(), key_dtype_, key_element_shape_,
                        dtypes_value_, element_shapes_value_, device);
    Tensor buf_indices, masks;
    new_hashmap.Insert(active_keys, soa_active_values, buf_indices, masks);

    return new_hashmap;
}

int64_t Hashmap::Size() const { return device_hashmap_->Size(); }

int64_t Hashmap::GetCapacity() const { return device_hashmap_->GetCapacity(); }

int64_t Hashmap::GetBucketCount() const {
    return device_hashmap_->GetBucketCount();
}

Device Hashmap::GetDevice() const { return device_hashmap_->GetDevice(); }

Tensor Hashmap::GetKeyTensor() const {
    int64_t capacity = GetCapacity();
    SizeVector key_shape = key_element_shape_;
    key_shape.insert(key_shape.begin(), capacity);
    return Tensor(key_shape, shape_util::DefaultStrides(key_shape),
                  device_hashmap_->GetKeyBuffer().GetDataPtr(), key_dtype_,
                  device_hashmap_->GetKeyBuffer().GetBlob());
}

std::vector<Tensor> Hashmap::GetValueTensors() const {
    int64_t capacity = GetCapacity();

    std::vector<Tensor> value_buffers = device_hashmap_->GetValueBuffers();

    std::vector<Tensor> soa_value_tensor;
    for (size_t i = 0; i < element_shapes_value_.size(); ++i) {
        SizeVector value_shape = element_shapes_value_[i];
        value_shape.insert(value_shape.begin(), capacity);

        Dtype value_dtype = dtypes_value_[i];
        soa_value_tensor.push_back(
                Tensor(value_shape, shape_util::DefaultStrides(value_shape),
                       value_buffers[i].GetDataPtr(), value_dtype,
                       value_buffers[i].GetBlob()));
    }
    return soa_value_tensor;
}

Tensor Hashmap::GetValueTensor(size_t i) const {
    int64_t capacity = GetCapacity();

    if (i >= dtypes_value_.size()) {
        utility::LogError("Value index ({}) out of bound (>= {})", i,
                          dtypes_value_.size());
    }

    Tensor value_buffer = device_hashmap_->GetValueBuffer(i);

    SizeVector value_shape = element_shapes_value_[i];
    value_shape.insert(value_shape.begin(), capacity);

    Dtype value_dtype = dtypes_value_[i];
    return Tensor(value_shape, shape_util::DefaultStrides(value_shape),
                  value_buffer.GetDataPtr(), value_dtype,
                  value_buffer.GetBlob());
}

std::vector<int64_t> Hashmap::BucketSizes() const {
    return device_hashmap_->BucketSizes();
};

float Hashmap::LoadFactor() const { return device_hashmap_->LoadFactor(); }

void Hashmap::Init(int64_t init_capacity,
                   const Device& device,
                   const HashBackendType& backend) {
    // Key check
    if (key_dtype_.GetDtypeCode() == Dtype::DtypeCode::Undefined) {
        utility::LogError("[Hashmap] Undefined key dtype is not allowed.");
    }
    if (key_element_shape_.NumElements() == 0) {
        utility::LogError(
                "[Hashmap] Key element shape must contain at least 1 element, "
                "but got 0.");
    }

    // Value check
    if (dtypes_value_.size() != element_shapes_value_.size()) {
        utility::LogError(
                "[Hashmap] Size of value_dtype ({}) mismatches with size of "
                "element_shapes_value ({}).",
                dtypes_value_.size(), element_shapes_value_.size());
    }
    for (auto& value_dtype : dtypes_value_) {
        if (value_dtype.GetDtypeCode() == Dtype::DtypeCode::Undefined) {
            utility::LogError(
                    "[Hashmap] Undefined value dtype is not allowed.");
        }
    }
    for (auto& value_element_shape : element_shapes_value_) {
        if (value_element_shape.NumElements() == 0) {
            utility::LogError(
                    "[Hashmap] Value element shape must contain at least 1 "
                    "element, but got 0.");
        }
    }

    device_hashmap_ = CreateDeviceHashBackend(
            init_capacity, key_dtype_, key_element_shape_, dtypes_value_,
            element_shapes_value_, device, backend);
}

void Hashmap::CheckKeyLength(const Tensor& input_keys) const {
    int64_t key_len = input_keys.GetLength();
    if (key_len == 0) {
        utility::LogError("Input number of keys should > 0, but got 0.");
    }
}

void Hashmap::CheckKeyValueLengthCompatibility(
        const Tensor& input_keys,
        const std::vector<Tensor>& input_values_soa) const {
    int64_t key_len = input_keys.GetLength();
    if (key_len == 0) {
        utility::LogError("Input number of keys should > 0, but got 0.");
    }
    for (size_t i = 0; i < input_values_soa.size(); ++i) {
        Tensor input_value = input_values_soa[i];
        if (input_value.GetLength() != key_len) {
            utility::LogError(
                    "Input number of values at {} mismatch with number of "
                    "keys "
                    "{}",
                    key_len, input_value.GetLength());
        }
    }
}

void Hashmap::CheckKeyCompatibility(const Tensor& input_keys) const {
    SizeVector input_key_elem_shape(input_keys.GetShape());
    input_key_elem_shape.erase(input_key_elem_shape.begin());

    int64_t input_key_elem_bytesize = input_key_elem_shape.NumElements() *
                                      input_keys.GetDtype().ByteSize();
    int64_t stored_key_elem_bytesize =
            key_element_shape_.NumElements() * key_dtype_.ByteSize();
    if (input_key_elem_bytesize != stored_key_elem_bytesize) {
        utility::LogError(
                "Input key element bytesize ({}) mismatch with stored ({})",
                input_key_elem_bytesize, stored_key_elem_bytesize);
    }
}

void Hashmap::CheckValueCompatibility(
        const std::vector<Tensor>& input_values_soa) const {
    if (input_values_soa.size() != element_shapes_value_.size()) {
        utility::LogError(
                "Input number of value arrays ({}) mismatches with stored "
                "({})",
                input_values_soa.size() != element_shapes_value_.size());
    }

    for (size_t i = 0; i < input_values_soa.size(); ++i) {
        Tensor input_value = input_values_soa[i];
        SizeVector input_value_i_elem_shape(input_value.GetShape());
        input_value_i_elem_shape.erase(input_value_i_elem_shape.begin());

        int64_t input_value_i_elem_bytesize =
                input_value_i_elem_shape.NumElements() *
                input_value.GetDtype().ByteSize();

        int64_t stored_value_i_elem_bytesize =
                element_shapes_value_[i].NumElements() *
                dtypes_value_[i].ByteSize();
        if (input_value_i_elem_bytesize != stored_value_i_elem_bytesize) {
            utility::LogError(
                    "Input value[{}] element bytesize ({}) mismatch with "
                    "stored ({})",
                    i, input_value_i_elem_bytesize,
                    stored_value_i_elem_bytesize);
        }
    }
}

void Hashmap::PrepareIndicesOutput(Tensor& output_buf_indices,
                                   int64_t length) const {
    if (output_buf_indices.GetLength() != length ||
        output_buf_indices.GetDtype() != core::Int32 ||
        output_buf_indices.GetDevice() != GetDevice()) {
        output_buf_indices = Tensor({length}, core::Int32, GetDevice());
    }
}

void Hashmap::PrepareMasksOutput(Tensor& output_masks, int64_t length) const {
    if (output_masks.GetLength() != length ||
        output_masks.GetDtype() != core::Bool ||
        output_masks.GetDevice() != GetDevice()) {
        output_masks = Tensor({length}, core::Bool, GetDevice());
    }
}

Hashset::Hashset(int64_t init_capacity,
                 const Dtype& key_dtype,
                 const SizeVector& key_element_shape,
                 const Device& device,
                 const HashBackendType& backend)
    : Hashmap(init_capacity,
              key_dtype,
              key_element_shape,
              std::vector<Dtype>{},
              std::vector<SizeVector>{},
              device,
              backend) {}

Hashset::Hashset(int64_t init_capacity,
                 const Dtype& key_dtype,
                 const SizeVector& key_element_shape,
                 const Dtype& value_dtype,
                 const SizeVector& value_element_shape,
                 const Device& device,
                 const HashBackendType& backend)
    : Hashmap(init_capacity,
              key_dtype,
              key_element_shape,
              std::vector<Dtype>{},
              std::vector<SizeVector>{},
              device,
              backend) {
    utility::LogError(
            "A hash set does not accept values. Please use a hash map "
            "instead.");
}

Hashset::Hashset(int64_t init_capacity,
                 const Dtype& key_dtype,
                 const SizeVector& key_element_shape,
                 const std::vector<Dtype>& dtypes_value,
                 const std::vector<SizeVector>& element_shapes_value,
                 const Device& device,
                 const HashBackendType& backend)
    : Hashmap(init_capacity,
              key_dtype,
              key_element_shape,
              std::vector<Dtype>{},
              std::vector<SizeVector>{},
              device,
              backend) {
    if (dtypes_value.size() != 0 || element_shapes_value.size() != 0) {
        utility::LogError(
                "A hash set does not accept values. Please use a hash map "
                "instead.");
    }
}

void Hashset::Insert(const Tensor& input_keys,
                     Tensor& output_buf_indices,
                     Tensor& output_masks) {
    return Hashmap::Insert(input_keys, std::vector<Tensor>{},
                           output_buf_indices, output_masks);
}
}  // namespace core
}  // namespace open3d
