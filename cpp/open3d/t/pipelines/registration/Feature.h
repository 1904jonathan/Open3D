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

namespace open3d {
namespace t {

namespace geometry {
class PointCloud;
}

namespace pipelines {
namespace registration {

/// Function to compute FPFH feature for a point cloud.
/// It uses KNN search if only max_nn parameter is provided, Radius search if
/// only radius parameter is provided, and Hybrid search if both are provided.
///
/// \param input The input point cloud with data type float32 ot float64.
/// \param max_nn [optional] Neighbor search max neighbors parameter. [Default =
/// 100].
/// \param radius [optional] Neighbor search radius parameter. [Recommended ~5x
/// voxel size]. 
/// \return A Tensor of FPFH feature of the input point cloud with
/// shape {N, 33}, data type and device same as input.
core::Tensor ComputeFPFHFeature(
        const geometry::PointCloud &input,
        const utility::optional<int> max_nn = 100,
        const utility::optional<double> radius = utility::nullopt);

}  // namespace registration
}  // namespace pipelines
}  // namespace t
}  // namespace open3d
