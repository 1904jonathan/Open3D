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

#include "open3d/t/geometry/VoxelBlockGrid.h"

#include "core/CoreTest.h"
#include "open3d/core/EigenConverter.h"
#include "open3d/core/Tensor.h"
#include "open3d/io/PinholeCameraTrajectoryIO.h"
#include "open3d/io/TriangleMeshIO.h"
#include "open3d/t/io/ImageIO.h"
#include "open3d/t/io/NumpyIO.h"
#include "open3d/visualization/utility/DrawGeometry.h"
// #include "tests/UnitTest.h"

namespace open3d {
namespace tests {

using namespace t::geometry;

class VoxelBlockGridPermuteDevices : public PermuteDevices {};
INSTANTIATE_TEST_SUITE_P(VoxelBlockGrid,
                         VoxelBlockGridPermuteDevices,
                         testing::ValuesIn(PermuteDevices::TestCases()));

static core::Tensor GetIntrinsicTensor() {
    camera::PinholeCameraIntrinsic intrinsic = camera::PinholeCameraIntrinsic(
            camera::PinholeCameraIntrinsicParameters::PrimeSenseDefault);
    auto focal_length = intrinsic.GetFocalLength();
    auto principal_point = intrinsic.GetPrincipalPoint();
    return core::Tensor::Init<double>(
            {{focal_length.first, 0, principal_point.first},
             {0, focal_length.second, principal_point.second},
             {0, 0, 1}});
}

static std::vector<core::Tensor> GetExtrinsicTensors() {
    // Extrinsics
    std::string trajectory_path =
            std::string(TEST_DATA_DIR) + "/RGBD/odometry.log";
    auto trajectory =
            io::CreatePinholeCameraTrajectoryFromFile(trajectory_path);

    std::vector<core::Tensor> extrinsics;
    for (size_t i = 0; i < trajectory->parameters_.size(); ++i) {
        Eigen::Matrix4d extrinsic = trajectory->parameters_[i].extrinsic_;
        core::Tensor extrinsic_t =
                core::eigen_converter::EigenMatrixToTensor(extrinsic);
        extrinsics.emplace_back(extrinsic_t);
    }

    return extrinsics;
}

static std::vector<core::HashBackendType> EnumerateBackends(
        const core::Device &device, bool include_slab = true) {
    std::vector<core::HashBackendType> backends;
    if (device.GetType() == core::Device::DeviceType::CUDA) {
        if (include_slab) {
            backends.push_back(core::HashBackendType::Slab);
        }
        backends.push_back(core::HashBackendType::StdGPU);
    } else {
        backends.push_back(core::HashBackendType::TBB);
    }
    return backends;
}

static VoxelBlockGrid Integrate(const core::HashBackendType &backend,
                                const core::Dtype &dtype,
                                const core::Device &device,
                                const int resolution) {
    core::Tensor intrinsic = GetIntrinsicTensor();
    std::vector<core::Tensor> extrinsics = GetExtrinsicTensors();
    const float depth_scale = 1000.0;
    const float depth_max = 3.0;

    auto vbg = VoxelBlockGrid({"tsdf", "weight", "color"},
                              {core::Float32, dtype, dtype}, {{1}, {1}, {3}},
                              3.0 / 512, resolution, 10000, device, backend);

    for (size_t i = 0; i < extrinsics.size(); ++i) {
        Image depth = t::io::CreateImageFromFile(
                              fmt::format("{}/RGBD/depth/{:05d}.png",
                                          std::string(TEST_DATA_DIR), i))
                              ->To(device);
        Image color = t::io::CreateImageFromFile(
                              fmt::format("{}/RGBD/color/{:05d}.jpg",
                                          std::string(TEST_DATA_DIR), i))
                              ->To(device);

        core::Tensor frustum_block_coords = vbg.GetUniqueBlockCoordinates(
                depth, intrinsic, extrinsics[i], depth_scale, depth_max);
        vbg.Integrate(frustum_block_coords, depth, color, intrinsic,
                      extrinsics[i]);
    }

    return vbg;
}

TEST_P(VoxelBlockGridPermuteDevices, Construct) {
    core::Device device = GetParam();
    std::vector<core::HashBackendType> backends = EnumerateBackends(device);

    for (auto backend : backends) {
        auto vbg = VoxelBlockGrid({"tsdf", "weight", "color"},
                                  {core::Float32, core::UInt16, core::UInt8},
                                  {{1}, {1}, {3}}, 3.0 / 512, 8,
                                  /*  init capacity = */ 10, device, backend);

        auto tsdf_tensor = vbg.GetAttribute("tsdf");
        auto weight_tensor = vbg.GetAttribute("weight");
        auto color_tensor = vbg.GetAttribute("color");

        EXPECT_EQ(tsdf_tensor.GetShape(), core::SizeVector({10, 8, 8, 8, 1}));
        EXPECT_EQ(tsdf_tensor.GetDtype(), core::Dtype::Float32);

        EXPECT_EQ(weight_tensor.GetShape(), core::SizeVector({10, 8, 8, 8, 1}));
        EXPECT_EQ(weight_tensor.GetDtype(), core::Dtype::UInt16);

        EXPECT_EQ(color_tensor.GetShape(), core::SizeVector({10, 8, 8, 8, 3}));
        EXPECT_EQ(color_tensor.GetDtype(), core::Dtype::UInt8);
    }
}

TEST_P(VoxelBlockGridPermuteDevices, Indexing) {
    core::Device device = GetParam();
    std::vector<core::HashBackendType> backends = EnumerateBackends(device);

    for (auto backend : backends) {
        auto vbg = VoxelBlockGrid({"tsdf", "weight", "color"},
                                  {core::Float32, core::UInt16, core::UInt8},
                                  {{1}, {1}, {3}}, 3.0 / 512, 2, 10, device,
                                  backend);

        auto hashmap = vbg.GetHashMap();

        // Unique Coordinates: (-1, 3, 2), (0, 2, 4), (1, 2, 3)
        core::Tensor keys = core::Tensor(
                std::vector<int>{-1, 3, 2, 0, 2, 4, -1, 3, 2, 0, 2, 4, 1, 2, 3},
                core::SizeVector{5, 3}, core::Dtype::Int32, device);

        core::Tensor buf_indices, masks;
        hashmap.Activate(keys, buf_indices, masks);
        buf_indices = buf_indices.IndexGet({masks});
        EXPECT_EQ(buf_indices.GetLength(), 3);

        // Non-flattened version, recommended for debugging
        int entries_per_block = 2 * 2 * 2;
        core::Tensor voxel_indices = vbg.GetVoxelIndices(buf_indices);
        EXPECT_EQ(voxel_indices.GetShape(),
                  core::SizeVector({4, 3 * entries_per_block}));

        core::Tensor voxel_coords = vbg.GetVoxelCoordinates(voxel_indices);
        EXPECT_EQ(voxel_coords.GetShape(),
                  core::SizeVector({3, 3 * entries_per_block}));

        // Flattened version, recommended for performance
        std::tie(voxel_coords, voxel_indices) =
                vbg.GetVoxelCoordinatesAndFlattenedIndices();
        EXPECT_EQ(voxel_coords.GetShape(),
                  core::SizeVector({3 * entries_per_block, 3}));
        EXPECT_EQ(voxel_indices.GetShape(),
                  core::SizeVector({3 * entries_per_block}));
    }
}

TEST_P(VoxelBlockGridPermuteDevices, GetUniqueBlockCoordinates) {
    core::Device device = GetParam();
    std::vector<core::HashBackendType> backends = EnumerateBackends(device);

    core::Tensor intrinsic = GetIntrinsicTensor();
    std::vector<core::Tensor> extrinsics = GetExtrinsicTensors();
    const float depth_scale = 1000.0;
    const float depth_max = 3.0;

    for (auto backend : backends) {
        auto vbg = VoxelBlockGrid({"tsdf", "weight", "color"},
                                  {core::Float32, core::Float32, core::UInt16},
                                  {{1}, {1}, {3}}, 3.0 / 512, 8, 10000, device,
                                  backend);

        const int i = 0;
        Image depth = t::io::CreateImageFromFile(
                              fmt::format("{}/RGBD/depth/{:05d}.png",
                                          std::string(TEST_DATA_DIR), i))
                              ->To(device);
        core::Tensor block_coords_from_depth = vbg.GetUniqueBlockCoordinates(
                depth, intrinsic, extrinsics[i], depth_scale, depth_max);

        PointCloud pcd = PointCloud::CreateFromDepthImage(
                depth, intrinsic, extrinsics[i], depth_scale, depth_max, 4);
        core::Tensor block_coords_from_pcd = vbg.GetUniqueBlockCoordinates(pcd);

        // Hard-coded result -- implementation could change,
        // freeze result of test_data when stable.
        EXPECT_EQ(block_coords_from_depth.GetLength(), 4873);
        EXPECT_EQ(block_coords_from_pcd.GetLength(), 6227);
    }
}

TEST_P(VoxelBlockGridPermuteDevices, Integrate) {
    core::Device device = GetParam();
    std::vector<core::HashBackendType> backends = EnumerateBackends(device);

    // Again, hard-coded result
    std::unordered_map<int, int> kResolutionPoints = {{8, 225628},
                                                      {16, 256717}};
    std::unordered_map<int, int> kResolutionVertices = {{8, 223075},
                                                        {16, 256234}};
    std::unordered_map<int, int> kResolutionTriangles = {{8, 409271},
                                                         {16, 493929}};

    for (auto backend : backends) {
        for (int block_resolution : std::vector<int>{8, 16}) {
            for (auto &dtype :
                 std::vector<core::Dtype>{core::Float32, core::UInt16}) {
                auto vbg = Integrate(backend, dtype, device, block_resolution);

                // Allow numerical precision differences
                auto pcd = vbg.ExtractPointCloud();
                EXPECT_NEAR(pcd.GetPointPositions().GetLength(),
                            kResolutionPoints[block_resolution], 3);

                auto mesh = vbg.ExtractTriangleMesh();
                EXPECT_NEAR(mesh.GetVertexPositions().GetLength(),
                            kResolutionVertices[block_resolution], 3);
                EXPECT_NEAR(mesh.GetTriangleIndices().GetLength(),
                            kResolutionTriangles[block_resolution], 6);
            }
        }
    }
}

TEST_P(VoxelBlockGridPermuteDevices, RayCasting) {
    core::Device device = GetParam();
    std::vector<core::HashBackendType> backends =
            EnumerateBackends(device, /* include_slab = */ false);

    core::Tensor intrinsic = GetIntrinsicTensor();
    std::vector<core::Tensor> extrinsics = GetExtrinsicTensors();
    const float depth_scale = 1000.0;
    const float depth_min = 0.1;
    const float depth_max = 3.0;

    for (auto backend : backends) {
        for (auto &dtype :
             std::vector<core::Dtype>{core::Float32, core::UInt16}) {
            auto vbg = Integrate(backend, dtype, device,
                                 /* block_resolution = */ 8);

            int i = extrinsics.size() - 1;

            Image depth = t::io::CreateImageFromFile(
                                  fmt::format("{}/RGBD/depth/{:05d}.png",
                                              std::string(TEST_DATA_DIR), i))
                                  ->To(device);
            core::Tensor frustum_block_coords = vbg.GetUniqueBlockCoordinates(
                    depth, intrinsic, extrinsics[i], depth_scale, depth_max);

            // Select sets
            auto result_odometry =
                    vbg.RayCast(frustum_block_coords, intrinsic, extrinsics[i],
                                depth.GetCols(), depth.GetRows(),
                                {"vertex", "normal", "depth"}, depth_scale,
                                depth_min, depth_max, 1.0);
            EXPECT_TRUE(result_odometry.Contains("vertex"));
            EXPECT_TRUE(result_odometry.Contains("normal"));
            EXPECT_TRUE(result_odometry.Contains("depth"));

            auto result_rendering = vbg.RayCast(
                    frustum_block_coords, intrinsic, extrinsics[i],
                    depth.GetCols(), depth.GetRows(), {"depth", "color"},
                    depth_scale, depth_min, depth_max, 1.0);
            EXPECT_TRUE(result_rendering.Contains("depth"));
            EXPECT_TRUE(result_rendering.Contains("color"));

            auto result_diff_rendering =
                    vbg.RayCast(frustum_block_coords, intrinsic, extrinsics[i],
                                depth.GetCols(), depth.GetRows(),
                                {"mask", "ratio", "index", "grad_ratio_x",
                                 "grad_ratio_y", "grad_ratio_z"},
                                depth_scale, depth_min, depth_max, 1.0);
            EXPECT_TRUE(result_diff_rendering.Contains("mask"));
            EXPECT_TRUE(result_diff_rendering.Contains("ratio"));
            EXPECT_TRUE(result_diff_rendering.Contains("index"));
            EXPECT_TRUE(result_diff_rendering.Contains("grad_ratio_x"));
            EXPECT_TRUE(result_diff_rendering.Contains("grad_ratio_y"));
            EXPECT_TRUE(result_diff_rendering.Contains("grad_ratio_z"));
        }
    }
}

}  // namespace tests
}  // namespace open3d
