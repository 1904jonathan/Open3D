// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// Copyright (c) 2018-2024 www.open3d.org
// SPDX-License-Identifier: MIT
// ----------------------------------------------------------------------------

#include <Eigen/Dense>
#include <unordered_set>

#include "open3d/geometry/KDTreeFlann.h"
#include "open3d/geometry/PointCloud.h"
#include "open3d/utility/Logging.h"
#include "open3d/utility/Parallel.h"
#include "open3d/utility/ProgressBar.h"

namespace open3d {
namespace geometry {

std::vector<int> PointCloud::ClusterDBSCAN(double eps,
                                           size_t min_points,
                                           bool print_progress) const {
    KDTreeFlann kdtree(*this);

    // Precompute all neighbors.
    utility::LogDebug("Precompute neighbors.");
    utility::ProgressBar progress_bar(points_.size(), "Precompute neighbors.",
                                      print_progress);
    std::vector<std::vector<int>> nbs(points_.size());
#pragma omp parallel for schedule(static) \
        num_threads(utility::EstimateMaxThreads())
    for (int idx = 0; idx < int(points_.size()); ++idx) {
        std::vector<double> dists2;
        kdtree.SearchRadius(points_[idx], eps, nbs[idx], dists2);

#pragma omp critical(ClusterDBSCAN)
        { ++progress_bar; }
    }
    utility::LogDebug("Done Precompute neighbors.");

    // Set all labels to undefined (-2).
    utility::LogDebug("Compute Clusters");
    progress_bar.Reset(points_.size(), "Clustering", print_progress);
    std::vector<int> labels(points_.size(), -2);
    int cluster_label = 0;
    for (size_t idx = 0; idx < points_.size(); ++idx) {
        // Label is not undefined.
        if (labels[idx] != -2) {
            continue;
        }

        // Check density.
        if (nbs[idx].size() < min_points) {
            labels[idx] = -1;
            continue;
        }

        labels[idx] = cluster_label;
        ++progress_bar;

        std::vector<int> nbs_next(nbs[idx].begin() + 1, nbs[idx].end());

        while (!nbs_next.empty()) {
            int nb = nbs_next.back();
            nbs_next.pop_back();

            if (labels[nb] >= 0) {
                continue;
            }
            labels[nb] = cluster_label;
            ++progress_bar;

            if (nbs[nb].size() >= min_points) {
                for (int qnb : nbs[nb]) {
                    if (labels[qnb] < 0) {
                        nbs_next.push_back(qnb);
                    }
                }
            }
        }

        cluster_label++;
    }

    utility::LogDebug("Done Compute Clusters: {:d}", cluster_label);
    return labels;
}

}  // namespace geometry
}  // namespace open3d
