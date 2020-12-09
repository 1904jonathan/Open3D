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

#include "open3d/t/pipelines/registration/TransformationEstimation.h"

#include <Eigen/Geometry>

#include "open3d/core/Tensor.h"
#include "open3d/t/geometry/PointCloud.h"
#include "open3d/utility/Eigen.h"

namespace open3d {
namespace t {
namespace pipelines {
namespace registration {

double TransformationEstimationPointToPoint::ComputeRMSE(
        const geometry::PointCloud &source,
        const geometry::PointCloud &target,
        const core::Tensor &corres,
        const int corres_num) const {
    utility::LogError("Unimplemented");
    double err = 0.0;
    return err;
}

core::Tensor TransformationEstimationPointToPoint::ComputeTransformation(
        const geometry::PointCloud &source,
        const geometry::PointCloud &target,
        const core::Tensor &corres,
        const int corres_num) const {
    utility::LogError("Unimplemented");
    return core::Tensor::Eye(4, core::Dtype::Float64, core::Device("CPU:0"));
}

double TransformationEstimationPointToPlane::ComputeRMSE(
        const geometry::PointCloud &source,
        const geometry::PointCloud &target,
        const core::Tensor &corres,
        const int corres_num) const {
    utility::LogError("Unimplemented");
    double err = 0.0;
    return err;
}

core::Tensor TransformationEstimationPointToPlane::ComputeTransformation(
        const geometry::PointCloud &source,
        const geometry::PointCloud &target,
        const core::Tensor &corres,
        const int corres_num) const {
    utility::LogError("Unimplemented");
    return core::Tensor::Eye(4, core::Dtype::Float64, core::Device("CPU:0"));
}

}  // namespace registration
}  // namespace pipelines
}  // namespace t
}  // namespace open3d
