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

#include "UnitTest.h"

#include "Core/Camera/PinholeCameraIntrinsic.h"

using namespace open3d;
using namespace std;
using namespace unit_test;

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
TEST(PinholeCameraIntrinsic, Constructor_Default)
{
    open3d::PinholeCameraIntrinsic intrinsic;

    EXPECT_EQ(-1, intrinsic.width_);
    EXPECT_EQ(-1, intrinsic.height_);

    intrinsic.intrinsic_matrix_ << 0.0, 0.0, 0.0,
                                   0.0, 0.0, 0.0,
                                   0.0, 0.0, 0.0;

    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(0, 0), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(1, 0), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(2, 0), THRESHOLD_1E_6);

    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(0, 1), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(1, 1), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(2, 1), THRESHOLD_1E_6);

    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(0, 2), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(1, 2), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(2, 2), THRESHOLD_1E_6);
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
TEST(PinholeCameraIntrinsic, Constructor_PrimeSenseDefault)
{
    open3d::PinholeCameraIntrinsic intrinsic(
        PinholeCameraIntrinsicParameters::PrimeSenseDefault);

    EXPECT_EQ(640, intrinsic.width_);
    EXPECT_EQ(480, intrinsic.height_);

    EXPECT_NEAR(525, intrinsic.intrinsic_matrix_(0, 0), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(1, 0), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(2, 0), THRESHOLD_1E_6);

    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(0, 1), THRESHOLD_1E_6);
    EXPECT_NEAR(525, intrinsic.intrinsic_matrix_(1, 1), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(2, 1), THRESHOLD_1E_6);

    EXPECT_NEAR(319.5, intrinsic.intrinsic_matrix_(0, 2), THRESHOLD_1E_6);
    EXPECT_NEAR(239.5, intrinsic.intrinsic_matrix_(1, 2), THRESHOLD_1E_6);
    EXPECT_NEAR(1.0, intrinsic.intrinsic_matrix_(2, 2), THRESHOLD_1E_6);
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
TEST(PinholeCameraIntrinsic, Constructor_Kinect2DepthCameraDefault)
{
    open3d::PinholeCameraIntrinsic intrinsic(
        PinholeCameraIntrinsicParameters::Kinect2DepthCameraDefault);

    EXPECT_EQ(512, intrinsic.width_);
    EXPECT_EQ(424, intrinsic.height_);

    EXPECT_NEAR(254.878, intrinsic.intrinsic_matrix_(0, 0), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(1, 0), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(2, 0), THRESHOLD_1E_6);

    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(0, 1), THRESHOLD_1E_6);
    EXPECT_NEAR(205.395, intrinsic.intrinsic_matrix_(1, 1), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(2, 1), THRESHOLD_1E_6);

    EXPECT_NEAR(365.456, intrinsic.intrinsic_matrix_(0, 2), THRESHOLD_1E_6);
    EXPECT_NEAR(365.456, intrinsic.intrinsic_matrix_(1, 2), THRESHOLD_1E_6);
    EXPECT_NEAR(1.0, intrinsic.intrinsic_matrix_(2, 2), THRESHOLD_1E_6);
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
TEST(PinholeCameraIntrinsic, Constructor_Kinect2ColorCameraDefault)
{
    open3d::PinholeCameraIntrinsic intrinsic(
        PinholeCameraIntrinsicParameters::Kinect2ColorCameraDefault);

    EXPECT_EQ(1920, intrinsic.width_);
    EXPECT_EQ(1080, intrinsic.height_);

    EXPECT_NEAR(1059.9718, intrinsic.intrinsic_matrix_(0, 0), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(1, 0), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(2, 0), THRESHOLD_1E_6);

    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(0, 1), THRESHOLD_1E_6);
    EXPECT_NEAR(1059.9718, intrinsic.intrinsic_matrix_(1, 1), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(2, 1), THRESHOLD_1E_6);

    EXPECT_NEAR(975.7193, intrinsic.intrinsic_matrix_(0, 2), THRESHOLD_1E_6);
    EXPECT_NEAR(545.9533, intrinsic.intrinsic_matrix_(1, 2), THRESHOLD_1E_6);
    EXPECT_NEAR(1.0, intrinsic.intrinsic_matrix_(2, 2), THRESHOLD_1E_6);
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
TEST(PinholeCameraIntrinsic, Constructor_Init)
{
    int width = 640;
    int height = 480;

    double fx = 0.5;
    double fy = 0.65;

    double cx = 0.75;
    double cy = 0.35;

    open3d::PinholeCameraIntrinsic intrinsic(width,
                                             height,
                                             fx, fy,
                                             cx, cy);

    EXPECT_EQ(width, intrinsic.width_);
    EXPECT_EQ(height, intrinsic.height_);

    EXPECT_NEAR(fx, intrinsic.intrinsic_matrix_(0, 0), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(1, 0), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(2, 0), THRESHOLD_1E_6);

    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(0, 1), THRESHOLD_1E_6);
    EXPECT_NEAR(fy, intrinsic.intrinsic_matrix_(1, 1), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(2, 1), THRESHOLD_1E_6);

    EXPECT_NEAR(cx, intrinsic.intrinsic_matrix_(0, 2), THRESHOLD_1E_6);
    EXPECT_NEAR(cy, intrinsic.intrinsic_matrix_(1, 2), THRESHOLD_1E_6);
    EXPECT_NEAR(1.0, intrinsic.intrinsic_matrix_(2, 2), THRESHOLD_1E_6);
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
TEST(PinholeCameraIntrinsic, DISABLED_MemberData)
{
    unit_test::NotImplemented();
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
TEST(PinholeCameraIntrinsic, SetIntrinsics)
{
    open3d::PinholeCameraIntrinsic intrinsic;

    EXPECT_EQ(-1, intrinsic.width_);
    EXPECT_EQ(-1, intrinsic.height_);

    intrinsic.intrinsic_matrix_ << 0.0, 0.0, 0.0,
                                   0.0, 0.0, 0.0,
                                   0.0, 0.0, 0.0;

    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(0, 0), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(1, 0), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(2, 0), THRESHOLD_1E_6);

    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(0, 1), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(1, 1), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(2, 1), THRESHOLD_1E_6);

    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(0, 2), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(1, 2), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(2, 2), THRESHOLD_1E_6);

    int width = 640;
    int height = 480;

    double fx = 0.5;
    double fy = 0.65;

    double cx = 0.75;
    double cy = 0.35;

    intrinsic.SetIntrinsics(width, height, fx, fy, cx, cy);

    EXPECT_EQ(width, intrinsic.width_);
    EXPECT_EQ(height, intrinsic.height_);

    EXPECT_NEAR(fx, intrinsic.intrinsic_matrix_(0, 0), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(1, 0), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(2, 0), THRESHOLD_1E_6);

    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(0, 1), THRESHOLD_1E_6);
    EXPECT_NEAR(fy, intrinsic.intrinsic_matrix_(1, 1), THRESHOLD_1E_6);
    EXPECT_NEAR(0.0, intrinsic.intrinsic_matrix_(2, 1), THRESHOLD_1E_6);

    EXPECT_NEAR(cx, intrinsic.intrinsic_matrix_(0, 2), THRESHOLD_1E_6);
    EXPECT_NEAR(cy, intrinsic.intrinsic_matrix_(1, 2), THRESHOLD_1E_6);
    EXPECT_NEAR(1.0, intrinsic.intrinsic_matrix_(2, 2), THRESHOLD_1E_6);
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
TEST(PinholeCameraIntrinsic, DISABLED_GetFocalLength)
{
    unit_test::NotImplemented();
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
TEST(PinholeCameraIntrinsic, DISABLED_GetPrincipalPoint)
{
    unit_test::NotImplemented();
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
TEST(PinholeCameraIntrinsic, DISABLED_GetSkew)
{
    unit_test::NotImplemented();
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
TEST(PinholeCameraIntrinsic, DISABLED_IsValid)
{
    unit_test::NotImplemented();
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
TEST(PinholeCameraIntrinsic, DISABLED_ConvertToJsonValue)
{
    unit_test::NotImplemented();
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
TEST(PinholeCameraIntrinsic, DISABLED_ConvertFromJsonValue)
{
    unit_test::NotImplemented();
}