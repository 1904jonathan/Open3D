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

#include <json/json.h>
#include <cstdio>

#include "Open3D/Geometry/Octree.h"
#include "Open3D/Geometry/PointCloud.h"
#include "Open3D/IO/ClassIO/OctreeIO.h"
#include "Open3D/IO/ClassIO/PointCloudIO.h"
#include "Open3D/Utility/IJsonConvertible.h"
#include "TestUtility/UnitTest.h"

using namespace open3d;
using namespace unit_test;

TEST(OctreeIO, JsonFileIOFragment) {
    // Create octree
    geometry::PointCloud pcd;
    io::ReadPointCloud(std::string(TEST_DATA_DIR) + "/fragment.ply", pcd);
    size_t max_depth = 6;
    geometry::Octree src_octree(max_depth);
    src_octree.ConvertFromPointCloud(pcd, true, 0.01);

    // Write to file
    std::string file_name =
            std::string(TEST_DATA_DIR) + "/fragment_octree.json";
    EXPECT_TRUE(io::WriteOctree(file_name, src_octree));

    // Read from file
    geometry::Octree dst_octree;
    EXPECT_TRUE(io::ReadOctree(file_name, dst_octree));
    EXPECT_TRUE(src_octree == dst_octree);
    EXPECT_EQ(std::remove(file_name.c_str()), 0);
}

TEST(Octree, JsonFileIOEightCubes) {
    // Build octree
    std::vector<Eigen::Vector3d> points{
            Eigen::Vector3d(0.5, 0.5, 0.5), Eigen::Vector3d(1.5, 0.5, 0.5),
            Eigen::Vector3d(0.5, 1.5, 0.5), Eigen::Vector3d(1.5, 1.5, 0.5),
            Eigen::Vector3d(0.5, 0.5, 1.5), Eigen::Vector3d(1.5, 0.5, 1.5),
            Eigen::Vector3d(0.5, 1.5, 1.5), Eigen::Vector3d(1.5, 1.5, 1.5),
    };
    std::vector<Eigen::Vector3d> colors{
            Eigen::Vector3d(0.0, 0.0, 0.0),   Eigen::Vector3d(0.25, 0.0, 0.0),
            Eigen::Vector3d(0.0, 0.25, 0.0),  Eigen::Vector3d(0.25, 0.25, 0.0),
            Eigen::Vector3d(0.0, 0.0, 0.25),  Eigen::Vector3d(0.25, 0.0, 0.25),
            Eigen::Vector3d(0.0, 0.25, 0.25), Eigen::Vector3d(0.25, 0.25, 0.25),
    };
    geometry::Octree src_octree(1, Eigen::Vector3d(0, 0, 0), 2);
    for (size_t i = 0; i < points.size(); ++i) {
        src_octree.InsertPoint(points[i], colors[i]);
    }

    // Write to file
    std::string file_name =
            std::string(TEST_DATA_DIR) + "/eight_cubes_octree.json";
    EXPECT_TRUE(io::WriteOctree(file_name, src_octree));

    // Read from file
    geometry::Octree dst_octree;
    EXPECT_TRUE(io::ReadOctree(file_name, dst_octree));
    EXPECT_TRUE(src_octree == dst_octree);
    EXPECT_EQ(std::remove(file_name.c_str()), 0);
}
