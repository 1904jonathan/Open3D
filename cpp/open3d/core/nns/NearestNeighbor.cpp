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

#include "open3d/core/nns/NearestNeighbor.h"

#include "open3d/utility/Console.h"

namespace open3d {
namespace core {
namespace nns {

NearestNeighbor::~NearestNeighbor(){};

bool NearestNeighbor::SetIndex() {
    index_.reset(new NanoFlannIndex());
    return index_->SetTensorData(dataset_points_);
};
bool NearestNeighbor::KnnIndex() { return SetIndex(); };
bool NearestNeighbor::RadiusIndex() { return SetIndex(); };
bool NearestNeighbor::FixedRadiusIndex() { return SetIndex(); }
bool NearestNeighbor::HybridIndex() { return SetIndex(); };

std::pair<core::Tensor, core::Tensor> NearestNeighbor::KnnSearch(
        const core::Tensor& query_points, int knn) {
    if (!index_) {
        utility::LogError("[NearestNeighbor::KnnSearch] Index is not set");
    }
    return index_->SearchKnn(query_points, knn);
}

std::tuple<core::Tensor, core::Tensor, core::Tensor>
NearestNeighbor::RadiusSearch(const core::Tensor& query_points,
                              const std::vector<double>& radii) {
    if (!index_) {
        utility::LogError("[NearestNeighbor::RadiusSearch] Index is not set");
    }
    return index_->SearchRadius(query_points, radii);
}

std::tuple<core::Tensor, core::Tensor, core::Tensor>
NearestNeighbor::FixedRadiusSearch(const core::Tensor& query_points,
                                   double radius) {
    if (!index_) {
        utility::LogError(
                "[NearestNeighbor::FixedRadiusSearch] Index is not set");
    }
    return index_->SearchRadius(query_points, radius);
}

std::pair<core::Tensor, core::Tensor> NearestNeighbor::HybridSearch(
        const core::Tensor& query_points, double radius, int max_knn) {
    if (!index_) {
        utility::LogError("[NearestNeighbor::HybridSearch] Index is not set");
    }
    std::pair<core::Tensor, core::Tensor> result =
            index_->SearchKnn(query_points, max_knn);
    core::Tensor indices = result.first;
    core::Tensor distances = result.second;
    core::SizeVector size = distances.GetShape();

    std::vector<int64_t> indices_vec = indices.ToFlatVector<int64_t>();
    std::vector<double> distances_vec = distances.ToFlatVector<double>();

    for (unsigned int i = 0; i < distances_vec.size(); i++) {
        if (distances_vec[i] > radius) {
            distances_vec[i] = 0;
            indices_vec[i] = -1;
        }
    }

    core::Tensor indices_new(indices_vec, size, core::Dtype::Int64);
    core::Tensor distances_new(distances_vec, size, core::Dtype::Float64);
    return std::make_pair(indices_new, distances_new);
}
}  // namespace nns
}  // namespace core
}  // namespace open3d
