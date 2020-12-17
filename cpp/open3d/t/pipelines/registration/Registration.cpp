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

#include "open3d/t/pipelines/registration/Registration.h"

#include "open3d/core/Tensor.h"
#include "open3d/core/nns/NearestNeighborSearch.h"
#include "open3d/t/geometry/PointCloud.h"
#include "open3d/utility/Console.h"
#include "open3d/utility/Helper.h"

namespace open3d {
namespace t {
namespace pipelines {
namespace registration {

static RegistrationResult GetRegistrationResultAndCorrespondences(
        const geometry::PointCloud &source,
        const geometry::PointCloud &target,
        open3d::core::nns::NearestNeighborSearch &target_nns,
        double max_correspondence_distance,
        const core::Tensor &transformation) {
    core::Device device = source.GetDevice();
    core::Dtype dtype = core::Dtype::Float32;
    source.GetPoints().AssertDtype(dtype);
    target.GetPoints().AssertDtype(dtype);
    if (target.GetDevice() != device) {
        utility::LogError(
                "Target Pointcloud device {} != Source Pointcloud's device {}.",
                target.GetDevice().ToString(), device.ToString());
    }
    transformation.AssertShape({4, 4});
    transformation.AssertDtype(dtype);
    core::Tensor transformation_device;
    if (transformation.GetDevice() == device) {
        transformation_device = transformation;
    } else {
        transformation_device = transformation.Copy(device);
    }

    RegistrationResult result(transformation_device);
    if (max_correspondence_distance <= 0.0) {
        return result;
    }

    bool check = target_nns.HybridIndex();
    if (!check) {
        utility::LogError(
                "[Tensor: EvaluateRegistration: "
                "GetRegistrationResultAndCorrespondences: "
                "NearestNeighborSearch::HybridSearch] "
                "Index is not set.");
    }
    // max_correspondece_dist in HybridSearch tensor implementation
    // is square root of that used in legacy implementation.
    // TODO: Inform author about this.
    max_correspondence_distance =
            max_correspondence_distance * max_correspondence_distance;

    auto result_nns = target_nns.HybridSearch(source.GetPoints(),
                                              max_correspondence_distance, 1);

    result.correspondence_select_bool_ =
            (result_nns.first.Ne(-1)).Reshape({-1});
    result.correspondence_set_ =
            result_nns.first.IndexGet({result.correspondence_select_bool_})
                    .Reshape({-1});
    core::Tensor dist_select =
            result_nns.second.IndexGet({result.correspondence_select_bool_})
                    .Reshape({-1});

    // Reduction sum of "distances" for error.
    auto squared_error = (dist_select.Sum({0})).Item<float_t>();
    result.fitness_ = (float)result.correspondence_set_.GetShape()[0] /
                      (float)result.correspondence_select_bool_.GetShape()[0];
    result.inlier_rmse_ = std::sqrt(
            squared_error / (float)result.correspondence_set_.GetShape()[0]);
    result.transformation_ = transformation;
    return result;
}

RegistrationResult EvaluateRegistration(const geometry::PointCloud &source,
                                        const geometry::PointCloud &target,
                                        double max_correspondence_distance,
                                        const core::Tensor &transformation /*
                                        = core::Tensor::Eye(4, 
                                        core::Dtype::Float32,
                                        core::Device("CPU:0"))*/) {
    core::Device device = source.GetDevice();
    core::Dtype dtype = core::Dtype::Float32;
    source.GetPoints().AssertDtype(dtype);
    target.GetPoints().AssertDtype(dtype);
    if (target.GetDevice() != device) {
        utility::LogError(
                "Target Pointcloud device {} != Source Pointcloud's device {}.",
                target.GetDevice().ToString(), device.ToString());
    }
    transformation.AssertShape({4, 4});
    transformation.AssertDtype(dtype);
    core::Tensor transformation_device;
    if (transformation.GetDevice() == device) {
        transformation_device = transformation;
    } else {
        transformation_device = transformation.Copy(device);
    }

    open3d::core::nns::NearestNeighborSearch target_nns(target.GetPoints());

    geometry::PointCloud source_transformed = source;
    // TODO: Check if transformation isIdentity (skip transform operation).
    source_transformed.Transform(transformation_device);
    return GetRegistrationResultAndCorrespondences(
            source_transformed, target, target_nns, max_correspondence_distance,
            transformation_device);
}

RegistrationResult RegistrationICP(const geometry::PointCloud &source,
                                   const geometry::PointCloud &target,
                                   double max_correspondence_distance,
                                   const core::Tensor &init,
                                   const TransformationEstimation &estimation,
                                   const ICPConvergenceCriteria &criteria) {
    core::Device device = source.GetDevice();
    core::Dtype dtype = core::Dtype::Float32;
    source.GetPoints().AssertDtype(dtype);
    target.GetPoints().AssertDtype(dtype);
    if (target.GetDevice() != device) {
        utility::LogError(
                "Target Pointcloud device {} != Source Pointcloud's device {}.",
                target.GetDevice().ToString(), device.ToString());
    }
    init.AssertShape({4, 4});
    init.AssertDtype(dtype);
    core::Tensor transformation_device;
    if (init.GetDevice() == device) {
        transformation_device = init;
    } else {
        transformation_device = init.Copy(device);
    }

    open3d::core::nns::NearestNeighborSearch target_nns(target.GetPoints());
    geometry::PointCloud source_transformed = source;

    // TODO: Check if transformation isIdentity (skip transform operation).
    source_transformed.Transform(transformation_device);
    // TODO: Default constructor absent in RegistrationResult class.
    RegistrationResult result(transformation_device);

    result = GetRegistrationResultAndCorrespondences(
            source_transformed, target, target_nns, max_correspondence_distance,
            transformation_device);
    auto corres = std::make_pair(result.correspondence_select_bool_,
                                 result.correspondence_set_);

    for (int i = 0; i < criteria.max_iteration_; i++) {
        utility::LogDebug("ICP Iteration #{:d}: Fitness {:.4f}, RMSE {:.4f}", i,
                          result.fitness_, result.inlier_rmse_);
        auto update = estimation.ComputeTransformation(source_transformed,
                                                       target, corres);
        transformation_device = update.Matmul(transformation_device);
        source_transformed.Transform(update);
        RegistrationResult backup = result;
        result = GetRegistrationResultAndCorrespondences(
                source_transformed, target, target_nns,
                max_correspondence_distance, transformation_device);
        corres = std::make_pair(result.correspondence_select_bool_,
                                result.correspondence_set_);
        if (std::abs(backup.fitness_ - result.fitness_) <
                    criteria.relative_fitness_ &&
            std::abs(backup.inlier_rmse_ - result.inlier_rmse_) <
                    criteria.relative_rmse_) {
            break;
        }
    }
    return result;
}

}  // namespace registration
}  // namespace pipelines
}  // namespace t
}  // namespace open3d
