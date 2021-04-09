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

#include "open3d/geometry/PlanarPatch.h"

#include <Eigen/Dense>
#include <algorithm>
#include <numeric>
#include <vector>

#include "open3d/geometry/BoundingVolume.h"
#include "open3d/geometry/PointCloud.h"
#include "open3d/utility/Console.h"

namespace open3d {
namespace geometry {

PlanarPatch& PlanarPatch::Clear() { return *this; }

bool PlanarPatch::IsEmpty() const { return false; }

Eigen::Vector3d PlanarPatch::GetMinBound() const {
    return center_ + basis_x_ + basis_y_;
}

Eigen::Vector3d PlanarPatch::GetMaxBound() const {
    return center_ - basis_x_ - basis_y_;
}

Eigen::Vector3d PlanarPatch::GetCenter() const { return center_; }

AxisAlignedBoundingBox PlanarPatch::GetAxisAlignedBoundingBox() const {
    AxisAlignedBoundingBox box;
    box.min_bound_ = GetMinBound();
    box.max_bound_ = GetMaxBound();
    return box;
}

OrientedBoundingBox PlanarPatch::GetOrientedBoundingBox() const {
    return OrientedBoundingBox::CreateFromAxisAlignedBoundingBox(
            GetAxisAlignedBoundingBox());
}

PlanarPatch& PlanarPatch::Transform(const Eigen::Matrix4d& transformation) {
    utility::LogError("Not implemented");
    return *this;
}

PlanarPatch& PlanarPatch::Translate(const Eigen::Vector3d& translation,
                                    bool relative) {
    utility::LogError("Not implemented");
    return *this;
}

PlanarPatch& PlanarPatch::Scale(const double scale,
                                const Eigen::Vector3d& center) {
    utility::LogError("Not implemented");
    return *this;
}

PlanarPatch& PlanarPatch::Rotate(const Eigen::Matrix3d& R,
                                 const Eigen::Vector3d& center) {
    utility::LogError("Not implemented");
    return *this;
}

PlanarPatch& PlanarPatch::PaintUniformColor(const Eigen::Vector3d& color) {
    color_ = color;
    return *this;
}

double PlanarPatch::GetSignedDistanceToPoint(
        const Eigen::Vector3d& point) const {
    return normal_.dot(point) + dist_from_origin_;
}

void PlanarPatch::OrientNormalToAlignWithDirection(
        const Eigen::Vector3d &orientation_reference
        /* = Eigen::Vector3d(0.0, 0.0, 1.0)*/) {

    if (normal_.dot(orientation_reference) < 0.0) {
        normal_ *= -1.0;
    }
}

void PlanarPatch::OrientNormalTowardsCameraLocation(
        const Eigen::Vector3d &camera_location /* = Eigen::Vector3d::Zero()*/) {
    const Eigen::Vector3d orientation_reference = camera_location - center_;
    OrientNormalToAlignWithDirection(orientation_reference);
}

}  // namespace geometry
}  // namespace open3d
