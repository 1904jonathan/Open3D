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

#include "open3d/tgeometry/PointCloud.h"

#include <Eigen/Core>
#include <string>
#include <unordered_map>

#include "open3d/core/ShapeUtil.h"
#include "open3d/core/Tensor.h"
#include "open3d/core/TensorList.h"

namespace open3d {
namespace tgeometry {

static Eigen::Vector3d TensorToEigenVector3d(const core::Tensor &tensor) {
    // TODO: Tensor::To(dtype, device).
    core::Tensor dtensor =
            tensor.To(core::Dtype::Float64).Copy(core::Device("CPU:0"));
    return Eigen::Vector3d(dtensor[0].Item<double>(), dtensor[1].Item<double>(),
                           dtensor[2].Item<double>());
}

static core::Tensor EigenVector3dToTensor(const Eigen::Vector3d &value,
                                          core::Dtype dtype,
                                          const core::Device &device) {
    return core::Tensor(value.data(), {3}, core::Dtype::Float64, device)
            .To(dtype);
}

PointCloud::PointCloud(const core::TensorList &points)
    : Geometry3D(Geometry::GeometryType::PointCloud),
      dtype_(points.GetDtype()),
      device_(points.GetDevice()) {
    points.AssertElementShape({3});
    point_attr_["points"] = points;
}

PointCloud::PointCloud(
        const std::unordered_map<std::string, core::TensorList> &point_dict)
    : Geometry3D(Geometry::GeometryType::PointCloud) {
    if (point_dict.find("points") == point_dict.end()) {
        utility::LogError("point_dict must have key \"points\".");
    }

    core::TensorList points = point_dict.at("points");
    points.AssertElementShape({3});
    dtype_ = points.GetDtype();
    device_ = points.GetDevice();

    for (auto &kv : point_dict) {
        if (device_ != kv.second.GetDevice()) {
            utility::LogError(
                    "points have device {}, however, a property has device {}.",
                    device_.ToString(), kv.second.GetDevice().ToString());
        }
        point_attr_.emplace(kv.first, kv.second);
    }
}

core::TensorList &PointCloud::operator[](const std::string &key) {
    return point_attr_.at(key);
}

void PointCloud::SyncPushBack(
        const std::unordered_map<std::string, core::Tensor> &point_struct) {
    // Check that the current tensorlists in point_attr_ have the same size.
    int64_t common_size = point_attr_.at("points").GetSize();
    for (auto &kv : point_attr_) {
        if (kv.second.GetSize() != common_size) {
            utility::LogError(
                    "Cannot perform SyncPushBack: \"points\" has length {}, "
                    "however, property \"{}\" has length {}.",
                    common_size, kv.second.GetSize());
        }
    }

    // If two maps have the same size, and one map contains all keys of the
    // other map, the two maps have exactly the same keys.
    if (point_attr_.size() != point_struct.size()) {
        utility::LogError(
                "Pointcloud's point_dict has size {} values, but the input "
                "point_struct has {} values.",
                point_attr_.size(), point_struct.size());
    }
    for (auto &kv : point_struct) {
        // TensorList::PushBack checks for dtype, shape, device.
        // point_attr_.at checks if the key exists.
        point_attr_.at(kv.first).PushBack(kv.second);
    }
}

PointCloud &PointCloud::Clear() {
    point_attr_.clear();
    return *this;
}

bool PointCloud::IsEmpty() const { return !HasPoints(); }

core::Tensor PointCloud::GetMinBound() const {
    point_attr_.at("points").AssertElementShape({3});
    return point_attr_.at("points").AsTensor().Min({0});
}

core::Tensor PointCloud::GetMaxBound() const {
    point_attr_.at("points").AssertElementShape({3});
    return point_attr_.at("points").AsTensor().Max({0});
}

core::Tensor PointCloud::GetCenter() const {
    point_attr_.at("points").AssertElementShape({3});
    return point_attr_.at("points").AsTensor().Mean({0});
}

PointCloud &PointCloud::Transform(const core::Tensor &transformation) {
    utility::LogError("Unimplemented");
    return *this;
}

PointCloud &PointCloud::Translate(const core::Tensor &translation,
                                  bool relative) {
    translation.AssertShape({3});
    core::Tensor transform = translation.Copy();
    if (!relative) {
        transform -= GetCenter();
    }
    point_attr_.at("points").AsTensor() += transform;
    return *this;
}

PointCloud &PointCloud::Scale(double scale, const core::Tensor &center) {
    center.AssertShape({3});
    point_attr_.at("points").AsTensor() =
            (point_attr_.at("points").AsTensor() - center) * scale + center;
    return *this;
}

PointCloud &PointCloud::Rotate(const core::Tensor &R,
                               const core::Tensor &center) {
    utility::LogError("Unimplemented");
    return *this;
}

tgeometry::PointCloud PointCloud::FromLegacyPointCloud(
        const geometry::PointCloud &pcd_legacy,
        core::Dtype dtype,
        const core::Device &device) {
    tgeometry::PointCloud pcd(dtype, device);
    if (pcd_legacy.HasPoints()) {
        pcd.point_attr_["points"] = core::TensorList({3}, dtype, device);
        // TODO: optimization always create host tensorlist first, and then copy
        // to device.
        for (const Eigen::Vector3d &point : pcd_legacy.points_) {
            pcd.point_attr_.at("points").PushBack(
                    EigenVector3dToTensor(point, dtype, device));
        }
    } else {
        utility::LogWarning(
                "Creating from an empty legacy pointcloud, an empty pointcloud "
                "with default dtype and device will be created.");
    }
    if (pcd_legacy.HasColors()) {
        pcd.point_attr_["colors"] = core::TensorList({3}, dtype, device);
        for (const Eigen::Vector3d &color : pcd_legacy.colors_) {
            pcd.point_attr_.at("colors").PushBack(
                    EigenVector3dToTensor(color, dtype, device));
        }
    }
    if (pcd_legacy.HasNormals()) {
        pcd.point_attr_["normals"] = core::TensorList({3}, dtype, device);
        for (const Eigen::Vector3d &normal : pcd_legacy.normals_) {
            pcd.point_attr_.at("normals").PushBack(
                    EigenVector3dToTensor(normal, dtype, device));
        }
    }
    return pcd;
}

geometry::PointCloud PointCloud::ToLegacyPointCloud() const {
    geometry::PointCloud pcd_legacy;
    if (HasPoints()) {
        core::TensorList points = point_attr_.at("points");
        for (int64_t i = 0; i < points.GetSize(); i++) {
            pcd_legacy.points_.push_back(TensorToEigenVector3d(points[i]));
        }
    }
    if (HasColors()) {
        core::TensorList colors = point_attr_.at("colors");
        for (int64_t i = 0; i < colors.GetSize(); i++) {
            pcd_legacy.colors_.push_back(TensorToEigenVector3d(colors[i]));
        }
    }
    if (HasNormals()) {
        core::TensorList normals = point_attr_.at("normals");
        for (int64_t i = 0; i < normals.GetSize(); i++) {
            pcd_legacy.normals_.push_back(TensorToEigenVector3d(normals[i]));
        }
    }
    return pcd_legacy;
}

}  // namespace tgeometry
}  // namespace open3d
