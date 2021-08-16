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
#include "open3d/t/geometry/TSDFVoxelGrid.h"
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
          float sdf_trunc,
          int block_resolution,
          int block_count,
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
    void SynthesizeModelFrame(Frame& raycast_frame,
                              float depth_scale,
                              float depth_min,
                              float depth_max,
                              bool enable_color = true);

    /// Track using PointToPlane depth odometry.
    /// TODO(wei): support hybrid or intensity methods.
    /// \param input_frame Input RGBD frame.
    /// \param raycast_frame RGBD frame generated by raycasting.
    /// \param depth_scale Scale factor to convert raw data into meter metric.
    /// \param depth_max Depth truncation to discard points far away from the
    /// camera.
    odometry::OdometryResult TrackFrameToModel(const Frame& input_frame,
                                               const Frame& raycast_frame,
                                               float depth_scale,
                                               float depth_max,
                                               float depth_diff);

    /// Integrate RGBD frame into the volumetric voxel grid.
    /// \param input_frame Input RGBD frame.
    /// \param depth_scale Scale factor to convert raw data into meter metric.
    /// \param depth_max Depth truncation to discard points far away from the
    /// camera.
    void Integrate(const Frame& input_frame,
                   float depth_scale,
                   float depth_max);

    /// Extract surface point cloud for visualization / model saving.
    /// \param estimated_number Estimation of the point cloud size, helpful for
    /// real-time visualization.
    /// \param weight_threshold Weight threshold of the TSDF voxels to prune
    /// noise.
    /// \return Extracted point cloud.
    t::geometry::PointCloud ExtractPointCloud(int estimated_number = -1,
                                              float weight_threshold = 3.0f);

    /// Extract surface triangle mesh for visualization / model saving.
    /// \param estimated_number Estimation of the point cloud size, helpful for
    /// real-time visualization.
    /// \param weight_threshold Weight threshold of the TSDF voxels to prune
    /// noise.
    /// \return Extracted point cloud.
    t::geometry::TriangleMesh ExtractTriangleMesh(
            int estimated_number = -1, float weight_threshold = 3.0f);

    /// Get block hashmap int the TSDFVoxelGrid.
    core::HashMap GetHashMap();

public:
    /// Maintained volumetric map.
    t::geometry::TSDFVoxelGrid voxel_grid_;

    /// T_frame_to_model, maintained tracking state in a (4, 4), Float64 Tensor
    /// on CPU.
    core::Tensor T_frame_to_world_;

    int frame_id_ = -1;
};
}  // namespace slam
}  // namespace pipelines
}  // namespace t
''
}  // namespace open3d
