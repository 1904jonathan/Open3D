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

#include "open3d/core/Tensor.h"
#include "open3d/t/geometry/Image.h"
#include "open3d/t/geometry/RGBDImage.h"
#include "open3d/t/geometry/VoxelBlockGrid.h"
#include "open3d/t/pipelines/odometry/RGBDOdometry.h"
#include "open3d/t/pipelines/slam/Frame.h"

namespace open3d {
namespace t {
namespace pipelines {
namespace slam {

/// Model class maintaining a volumetric grid and the current active frame's
/// pose. Wraps functionalities including integration, ray casting, and surface
/// reconstruction.
class Model {
public:
    Model() {}
    Model(float voxel_size,
          int block_resolution = 16,
          int block_count = 1000,
          const core::Tensor& T_init = core::Tensor::Eye(4,
                                                         core::Float64,
                                                         core::Device("CPU:0")),
          const core::Device& device = core::Device("CUDA:0"));

    core::Tensor GetCurrentFramePose() { return T_frame_to_world_; }
    void UpdateFramePose(int frame_id, const core::Tensor& T_frame_to_world) {
        if (frame_id != frame_id_ + 1) {
            utility::LogWarning("Skipped {} frames in update T!",
                                frame_id - (frame_id_ + 1));
        }
        frame_id_ = frame_id;
        T_frame_to_world_ = T_frame_to_world.Contiguous();
    }

    /// Apply ray casting to obtain a synthesized model frame at the down
    /// sampled resolution.
    /// \param raycast_frame RGBD frame to fill the raycasting results.
    /// \param depth_scale Scale factor to convert raw data into meter metric.
    /// \param depth_min Depth where ray casting starts from.
    /// \param depth_max Depth where ray casting stops at.
    /// \param trunc_voxel_multiplier Truncation distance multiplier in voxel
    /// size for signed distance. For instance, --trunc_voxel_multiplier=8 with
    /// --voxel_size=0.006(m) creates a truncation distance of 0.048(m).
    /// \param enable_color Enable color in the raycasting results.
    /// \param weight_threshold Weight threshold of the TSDF voxels to prune
    /// noise. Use -1 to apply default logic: min(frame_id_ * 1.0f, 3.0f).
    void SynthesizeModelFrame(Frame& raycast_frame,
                              float depth_scale = 1000.0,
                              float depth_min = 0.1,
                              float depth_max = 3.0,
                              float trunc_voxel_multiplier = 8.0,
                              bool enable_color = true,
                              float weight_threshold = -1.0);

    /// Track using depth odometry.
    /// \param input_frame Input RGBD frame.
    /// \param raycast_frame RGBD frame generated by raycasting.
    /// \param depth_scale Scale factor to convert raw data into meter metric.
    /// \param depth_max Depth truncation to discard points far away from the
    /// camera
    /// \param method Method used to apply RGBD odometry.
    /// \param criteria Criteria used to define and terminate iterations.
    /// In multiscale odometry the order is from coarse to fine. Inputting a
    /// vector of iterations by default triggers the implicit conversion.
    odometry::OdometryResult TrackFrameToModel(
            const Frame& input_frame,
            const Frame& raycast_frame,
            float depth_scale = 1000.0,
            float depth_max = 3.0,
            float depth_diff = 0.07,
            odometry::Method method = odometry::Method::PointToPlane,
            const std::vector<odometry::OdometryConvergenceCriteria>& criteria =
                    {6, 3, 1});

    /// Integrate RGBD frame into the volumetric voxel grid.
    /// \param input_frame Input RGBD frame.
    /// \param depth_scale Scale factor to convert raw data into meter metric.
    /// \param depth_max Depth truncation to discard points far away from the
    /// camera.
    /// \param trunc_voxel_multiplier Truncation distance multiplier in voxel
    /// size for signed distance. For instance, --trunc_voxel_multiplier=8 with
    /// --voxel_size=0.006(m) creates a truncation distance of 0.048(m).
    void Integrate(const Frame& input_frame,
                   float depth_scale = 1000.0,
                   float depth_max = 3.0,
                   float trunc_voxel_multiplier = 8.0f);

    /// Extract surface point cloud for visualization / model saving.
    /// \param weight_threshold Weight threshold of the TSDF voxels to prune
    /// noise.
    /// \param estimated_number Estimation of the point cloud size, helpful for
    /// real-time visualization.
    /// \return Extracted point cloud.
    t::geometry::PointCloud ExtractPointCloud(float weight_threshold = 3.0f,
                                              int estimated_number = -1);

    /// Extract surface triangle mesh for visualization / model saving.
    /// \param weight_threshold Weight threshold of the TSDF voxels to prune
    /// noise.
    /// \param estimated_number Estimation of the point cloud size, helpful for
    /// real-time visualization.
    /// \return Extracted point cloud.
    t::geometry::TriangleMesh ExtractTriangleMesh(float weight_threshold = 3.0f,
                                                  int estimated_number = -1);

    /// Get block hashmap int the VoxelBlockGrid.
    core::HashMap GetHashMap();

public:
    /// Maintained volumetric map.
    t::geometry::VoxelBlockGrid voxel_grid_;

    /// Active block coordinates from prior integration
    core::Tensor frustum_block_coords_;

    /// T_frame_to_model, maintained tracking state in a (4, 4), Float64 Tensor
    /// on CPU.
    core::Tensor T_frame_to_world_;

    int frame_id_ = -1;
};

}  // namespace slam
}  // namespace pipelines
}  // namespace t
}  // namespace open3d
