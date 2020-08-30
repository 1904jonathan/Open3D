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

#include "open3d/core/nns/NearestNeighborSearch.h"

#include "open3d/utility/Console.h"

namespace open3d {
namespace core {
namespace nns {

NearestNeighborSearch::~NearestNeighborSearch(){};

bool NearestNeighborSearch::SetIndex() {
    nanoflann_index_.reset(new NanoFlannIndex());
    return nanoflann_index_->SetTensorData(dataset_points_);
};
bool NearestNeighborSearch::KnnIndex() { return SetIndex(); };
bool NearestNeighborSearch::MultiRadiusIndex() { return SetIndex(); };
bool NearestNeighborSearch::FixedRadiusIndex() { return SetIndex(); }
bool NearestNeighborSearch::HybridIndex() { return SetIndex(); };

std::pair<core::Tensor, core::Tensor> NearestNeighborSearch::KnnSearch(
        const core::Tensor& query_points, int knn) {
    if (!nanoflann_index_) {
        utility::LogError(
                "[NearestNeighborSearch::KnnSearch] Index is not set");
    }
    return nanoflann_index_->SearchKnn(query_points, knn);
}

std::tuple<core::Tensor, core::Tensor, core::Tensor>
NearestNeighborSearch::FixedRadiusSearch(const core::Tensor& query_points,
                                         double radius) {
    if (!nanoflann_index_) {
        utility::LogError(
                "[NearestNeighborSearch::FixedRadiusSearch] Index is not set");
    }
    return nanoflann_index_->SearchRadius(query_points, radius);
}

std::tuple<core::Tensor, core::Tensor, core::Tensor>
NearestNeighborSearch::MultiRadiusSearch(const core::Tensor& query_points,
                                         const std::vector<double>& radii) {
    if (!nanoflann_index_) {
        utility::LogError(
                "[NearestNeighborSearch::MultiRadiusSearch] Index is not set");
    }
    return nanoflann_index_->SearchRadius(query_points, radii);
}

std::pair<core::Tensor, core::Tensor> NearestNeighborSearch::HybridSearch(
        const core::Tensor& query_points, double radius, int max_knn) {
    if (!nanoflann_index_) {
        utility::LogError(
                "[NearestNeighborSearch::HybridSearch] Index is not set");
    }
    std::pair<core::Tensor, core::Tensor> result =
            nanoflann_index_->SearchKnn(query_points, max_knn);
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
