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

#include "open3d/t/geometry/VoxelBlockGrid.h"

#include "open3d/core/Tensor.h"
#include "open3d/t/geometry/Geometry.h"
#include "open3d/t/geometry/PointCloud.h"
#include "open3d/t/geometry/kernel/TSDFVoxelGrid.h"
#include "open3d/t/geometry/kernel/VoxelBlockGrid.h"

namespace open3d {
namespace t {
namespace geometry {

VoxelBlockGrid::VoxelBlockGrid(
        const std::vector<std::string> &attr_names,
        const std::vector<core::Dtype> &attr_dtypes,
        const std::vector<core::SizeVector> &attr_channels,
        double voxel_size,
        int64_t block_resolution,
        int64_t block_count,
        const core::Device &device,
        const core::HashBackendType &backend)
    : voxel_size_(voxel_size), block_resolution_(block_resolution) {
    size_t n_attrs = attr_names.size();
    if (attr_dtypes.size() != n_attrs) {
        utility::LogError(
                "Number of attribute dtypes ({}) mismatch with names ({}).",
                attr_dtypes.size(), n_attrs);
    }
    if (attr_channels.size() != n_attrs) {
        utility::LogError(
                "Number of attribute channels ({}) mismatch with names ({}).",
                attr_channels.size(), n_attrs);
    }

    std::vector<core::SizeVector> attr_element_shapes;
    core::SizeVector block_shape{block_resolution, block_resolution,
                                 block_resolution};

    for (size_t i = 0; i < n_attrs; ++i) {
        // Construct element shapes.
        core::SizeVector attr_channel = attr_channels[i];
        core::SizeVector block_shape_copy = block_shape;
        block_shape_copy.insert(block_shape_copy.end(), attr_channel.begin(),
                                attr_channel.end());
        attr_element_shapes.emplace_back(block_shape_copy);

        // Used for easier accessing via attribute names.
        name_attr_map_[attr_names[i]] = i;
    }

    block_hashmap_ = std::make_shared<core::HashMap>(
            block_count, core::Int32, core::SizeVector{3}, attr_dtypes,
            attr_element_shapes, device, backend);
}

core::Tensor VoxelBlockGrid::GetAttribute(const std::string &attr_name) const {
    if (name_attr_map_.count(attr_name) == 0) {
        utility::LogWarning("Attribute {} not found, return empty tensor.",
                            attr_name);
    }
    int buffer_idx = name_attr_map_.at(attr_name);
    return block_hashmap_->GetValueTensor(buffer_idx);
}

core::Tensor VoxelBlockGrid::GetVoxelCoordinates(
        const core::Tensor &voxel_indices) const {
    core::Tensor key_tensor = block_hashmap_->GetKeyTensor();

    core::Tensor voxel_coords =
            key_tensor.IndexGet({voxel_indices[0]}).T().To(core::Int64) *
            block_resolution_;
    voxel_coords[0] += voxel_indices[1];
    voxel_coords[1] += voxel_indices[2];
    voxel_coords[2] += voxel_indices[3];

    return voxel_coords;
}

core::Tensor VoxelBlockGrid::GetVoxelIndices() const {
    core::Device device = block_hashmap_->GetDevice();

    core::Tensor active_buf_indices;
    block_hashmap_->GetActiveIndices(active_buf_indices);

    int64_t n_blocks = active_buf_indices.GetLength();

    int64_t resolution = block_resolution_;
    int64_t resolution2 = resolution * resolution;
    int64_t resolution3 = resolution2 * resolution;

    // Non-kernel version.
    /// TODO: Check if kernel version is necessary.
    core::Tensor linear_coordinates = core::Tensor::Arange(
            0, n_blocks * resolution3, 1, core::Int64, device);

    core::Tensor block_idx = linear_coordinates / resolution3;
    core::Tensor remainder = linear_coordinates - block_idx * resolution3;

    /// operator % is not supported now
    core::Tensor voxel_z = remainder / resolution2;
    core::Tensor voxel_y = (remainder - (voxel_z * resolution2)) / resolution;
    core::Tensor voxel_x = remainder - (remainder / resolution) * resolution;

    core::Tensor voxel_indices = core::Tensor({4, n_blocks * resolution3},
                                              core::Dtype::Int64, device);
    voxel_indices[0] =
            active_buf_indices.IndexGet({block_idx}).To(core::Dtype::Int64);
    voxel_indices[1] = voxel_x;
    voxel_indices[2] = voxel_y;
    voxel_indices[3] = voxel_z;

    return voxel_indices;
}

core::Tensor VoxelBlockGrid::GetUniqueBlockCoordinates(
        const Image &depth,
        const core::Tensor &intrinsics,
        const core::Tensor &extrinsics,
        float depth_scale,
        float depth_max) {
    if (depth.IsEmpty()) {
        utility::LogError("Input depth is empty.");
    }
    if (depth.GetDtype() != core::UInt16 && depth.GetDtype() != core::Float32) {
        utility::LogError("Unsupported depth image dtype {}.",
                          depth.GetDtype().ToString());
    }

    const int64_t down_factor = 4;
    if (frustum_hashmap_ == nullptr) {
        int64_t capacity = (depth.GetCols() * depth.GetRows()) /
                           (down_factor * down_factor * 4);
        frustum_hashmap_ = std::make_shared<core::HashMap>(
                capacity, core::Int32, core::SizeVector{3}, core::Int32,
                core::SizeVector{1}, block_hashmap_->GetDevice());
    } else {
        frustum_hashmap_->Clear();
    }

    core::Tensor block_coords;
    PointCloud pcd = PointCloud::CreateFromDepthImage(
            depth, intrinsics, extrinsics, depth_scale, depth_max, down_factor);
    kernel::voxel_grid::Touch(
            frustum_hashmap_, pcd.GetPointPositions().Contiguous(),
            block_coords, block_resolution_, voxel_size_, 6 * voxel_size_);

    return block_coords;
}

void VoxelBlockGrid::Integrate(const core::Tensor &block_coords,
                               const Image &depth,
                               const Image &color,
                               const core::Tensor &intrinsics,
                               const core::Tensor &extrinsics,
                               float depth_scale,
                               float depth_max) {
    core::Tensor buf_indices, masks;
    block_hashmap_->Activate(block_coords, buf_indices, masks);
    block_hashmap_->Find(block_coords, buf_indices, masks);

    core::Tensor block_keys = block_hashmap_->GetKeyTensor();
    std::vector<core::Tensor> block_values = block_hashmap_->GetValueTensors();
    kernel::voxel_grid::Integrate(
            depth.AsTensor(), color.AsTensor(), buf_indices, block_keys,
            block_values, intrinsics, extrinsics, block_resolution_,
            voxel_size_, voxel_size_ * 6, depth_scale, depth_max);
}

std::pair<core::Tensor, core::Tensor> BufferRadiusNeighbors(
        std::shared_ptr<core::HashMap> &hashmap,
        const core::Tensor &active_buf_indices) {
    // Fixed radius search for spatially hashed voxel blocks.
    // A generalization will be implementing dense/sparse fixed radius
    // search with coordinates as hashmap keys.
    core::Tensor key_buffer_int3_tensor = hashmap->GetKeyTensor();

    core::Tensor active_keys = key_buffer_int3_tensor.IndexGet(
            {active_buf_indices.To(core::Int64)});
    int64_t n = active_keys.GetShape()[0];

    // Fill in radius nearest neighbors.
    core::Tensor keys_nb({27, n, 3}, core::Int32, hashmap->GetDevice());
    for (int nb = 0; nb < 27; ++nb) {
        int dz = nb / 9;
        int dy = (nb % 9) / 3;
        int dx = nb % 3;
        core::Tensor dt =
                core::Tensor(std::vector<int>{dx - 1, dy - 1, dz - 1}, {1, 3},
                             core::Int32, hashmap->GetDevice());
        keys_nb[nb] = active_keys + dt;
    }
    keys_nb = keys_nb.View({27 * n, 3});

    core::Tensor buf_indices_nb, masks_nb;
    hashmap->Find(keys_nb, buf_indices_nb, masks_nb);
    return std::make_pair(buf_indices_nb.View({27, n, 1}),
                          masks_nb.View({27, n, 1}));
}

PointCloud VoxelBlockGrid::ExtractSurfacePoints(int estimated_number,
                                                float weight_threshold) {
    core::Tensor active_buf_indices;
    block_hashmap_->GetActiveIndices(active_buf_indices);

    core::Tensor active_nb_buf_indices, active_nb_masks;
    std::tie(active_nb_buf_indices, active_nb_masks) =
            BufferRadiusNeighbors(block_hashmap_, active_buf_indices);

    // Extract points around zero-crossings.
    core::Tensor points, normals, colors;

    core::Tensor block_keys = block_hashmap_->GetKeyTensor();
    std::vector<core::Tensor> block_values = block_hashmap_->GetValueTensors();
    kernel::voxel_grid::ExtractSurfacePoints(
            active_buf_indices, active_nb_buf_indices, active_nb_masks,
            block_keys, block_values, points, normals, colors,
            block_resolution_, voxel_size_, weight_threshold, estimated_number);

    auto pcd = PointCloud(points.Slice(0, 0, estimated_number));
    pcd.SetPointColors(colors.Slice(0, 0, estimated_number));
    // pcd.SetPointNormals(normals.Slice(0, 0, estimated_number));

    return pcd;
}

std::unordered_map<std::string, core::Tensor> VoxelBlockGrid::RayCast(
        const core::Tensor &block_coords,
        const core::Tensor &intrinsics,
        const core::Tensor &extrinsics,
        int width,
        int height,
        float depth_scale,
        float depth_min,
        float depth_max,
        float weight_threshold) {
    // Extrinsic: world to camera -> pose: camera to world
    core::Tensor vertex_map, depth_map, color_map, normal_map;
    core::Device device = block_hashmap_->GetDevice();
    vertex_map = core::Tensor({height, width, 3}, core::Float32, device);
    depth_map = core::Tensor({height, width, 1}, core::Float32, device);
    color_map = core::Tensor({height, width, 3}, core::Float32, device);

    const int down_factor = 8;
    core::Tensor range_minmax_map;
    kernel::tsdf::EstimateRange(block_coords, range_minmax_map, intrinsics,
                                extrinsics, height, width, down_factor,
                                block_resolution_, voxel_size_, depth_min,
                                depth_max);

    std::vector<core::Tensor> block_values = block_hashmap_->GetValueTensors();
    auto device_hashmap = block_hashmap_->GetDeviceHashBackend();
    kernel::voxel_grid::RayCast(
            device_hashmap, block_values, range_minmax_map, vertex_map,
            depth_map, color_map, normal_map, intrinsics, extrinsics, height,
            width, block_resolution_, voxel_size_, voxel_size_ * 6, depth_scale,
            depth_min, depth_max, weight_threshold);

    std::unordered_map<std::string, core::Tensor> results;
    results.emplace("vertex", vertex_map);
    results.emplace("depth", depth_map);
    results.emplace("color", color_map);
    results.emplace("range", range_minmax_map);

    return results;
}

}  // namespace geometry
}  // namespace t
}  // namespace open3d
