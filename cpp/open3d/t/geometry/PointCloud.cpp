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

#include "open3d/t/geometry/PointCloud.h"

#include <Eigen/Core>
#include <string>
#include <unordered_map>

#include "open3d/core/EigenConverter.h"
#include "open3d/core/ShapeUtil.h"
#include "open3d/core/Tensor.h"
#include "open3d/t/geometry/TensorMap.h"

namespace open3d {
namespace t {
namespace geometry {

PointCloud::PointCloud(const core::Device &device)
    : Geometry(Geometry::GeometryType::PointCloud, 3),
      device_(device),
      point_attr_(TensorMap("points")) {
    ;
}

PointCloud::PointCloud(const core::Tensor &points)
    : PointCloud(points.GetDevice()) {
    points.AssertShapeCompatible({utility::nullopt, 3});
    SetPoints(points);
}

PointCloud::PointCloud(const std::unordered_map<std::string, core::Tensor>
                               &map_keys_to_tensors)
    : PointCloud(map_keys_to_tensors.at("points").GetDevice()) {
    if (map_keys_to_tensors.count("points") == 0) {
        utility::LogError("\"points\" attribute must be specified.");
    }
    map_keys_to_tensors.at("points").AssertShapeCompatible(
            {utility::nullopt, 3});
    point_attr_ = TensorMap("points", map_keys_to_tensors.begin(),
                            map_keys_to_tensors.end());
}

core::Tensor PointCloud::GetMinBound() const { return GetPoints().Min({0}); }

core::Tensor PointCloud::GetMaxBound() const { return GetPoints().Max({0}); }

core::Tensor PointCloud::GetCenter() const { return GetPoints().Mean({0}); }

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
    GetPoints() += transform;
    return *this;
}

PointCloud &PointCloud::Scale(double scale, const core::Tensor &center) {
    center.AssertShape({3});
    core::Tensor points = GetPoints();
    points.Sub_(center).Mul_(scale).Add_(center);
    return *this;
}

PointCloud &PointCloud::Rotate(const core::Tensor &R,
                               const core::Tensor &center) {
    utility::LogError("Unimplemented");
    return *this;
}

geometry::PointCloud PointCloud::FromLegacyPointCloud(
        const open3d::geometry::PointCloud &pcd_legacy,
        core::Dtype dtype,
        const core::Device &device) {
    geometry::PointCloud pcd(device);
    if (pcd_legacy.HasPoints()) {
        pcd.SetPoints(core::eigen_converter::EigenVector3dVectorToTensor(
                pcd_legacy.points_, dtype, device));
    } else {
        utility::LogWarning("Creating from an empty legacy PointCloud.");
    }
    if (pcd_legacy.HasColors()) {
        pcd.SetPointColors(core::eigen_converter::EigenVector3dVectorToTensor(
                pcd_legacy.colors_, dtype, device));
    }
    if (pcd_legacy.HasNormals()) {
        pcd.SetPointNormals(core::eigen_converter::EigenVector3dVectorToTensor(
                pcd_legacy.normals_, dtype, device));
    }
    return pcd;
}

open3d::geometry::PointCloud PointCloud::ToLegacyPointCloud() const {
    open3d::geometry::PointCloud pcd_legacy;
    if (HasPoints()) {
        pcd_legacy.points_ =
                core::eigen_converter::TensorToEigenVector3dVector(GetPoints());
    }
    if (HasPointColors()) {
        pcd_legacy.colors_ = core::eigen_converter::TensorToEigenVector3dVector(
                GetPointColors());
    }
    if (HasPointNormals()) {
        pcd_legacy.normals_ =
                core::eigen_converter::TensorToEigenVector3dVector(
                        GetPointNormals());
    }
    return pcd_legacy;
}

}  // namespace geometry
}  // namespace t
}  // namespace open3d
