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
#include <memory>
#include <Eigen/Core>
#include <Core/Geometry/Geometry3D.h>

namespace open3d {

class Image;
class PointCloud;
class TriangleMesh;
class PinholeCameraParameters;

class VoxelGrid : public Geometry3D
{
public:
    VoxelGrid() : Geometry3D(Geometry::GeometryType::VoxelGrid){};
    ~VoxelGrid() override{};

public:
    void Clear() override;
    bool IsEmpty() const override;
    Eigen::Vector3d GetMinBound() const override;
    Eigen::Vector3d GetMaxBound() const override;
    void Transform(const Eigen::Matrix4d &transformation) override;

public:
    VoxelGrid &operator+=(const VoxelGrid &voxelgrid);
    VoxelGrid operator+(const VoxelGrid &voxelgrid) const;

public:
    bool HasVoxels() const {
        return voxels_.size() > 0;
    }
    bool HasColors() const {
        return voxels_.size() > 0 && colors_.size() == voxels_.size();
    }
    Eigen::Vector3d GetOriginalCoordinate(int id) const {
        return ((voxels_[id].cast<double>() + Eigen::Vector3d(0.5, 0.5, 0.5))
                * voxel_size_) + origin_;
    }
    std::vector<Eigen::Vector3d> GetBoundingPointsOfVoxel(int index);

public : 
    double voxel_size_;
    Eigen::Vector3d origin_;
    std::vector<Eigen::Vector3i> voxels_;
    std::vector<Eigen::Vector3d> colors_;

};

std::shared_ptr<VoxelGrid> CreateSurfaceVoxelGridFromPointCloud(
        const PointCloud &input, double voxel_size);

std::shared_ptr<VoxelGrid> CreateVoxelGrid(
        double w, double h, double d, double voxel_size,
        const Eigen::Vector3d origin);

std::shared_ptr<VoxelGrid> CarveVoxelGridUsingDepthMap (
        VoxelGrid &input, const Image &silhouette_mask,
        const PinholeCameraParameters &camera_parameter);

void CarveVoxelGridUsingSilhouette (
        VoxelGrid &input, const Image &silhouette_mask,
        const PinholeCameraParameters &camera_parameter);

void Test (const std::shared_ptr<PinholeCameraParameters> &camera_parameter);

}
