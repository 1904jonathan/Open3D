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

#include <json/json.h>

#include <Eigen/Geometry>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "open3d/data/Dataset.h"
#include "open3d/utility/IJsonConvertible.h"
#include "open3d/utility/Logging.h"

namespace open3d {
namespace data {

namespace {

bool ReadJSONFromFile(const std::string& path, Json::Value& json) {
    std::ifstream file(path);
    if (!file.is_open()) {
        utility::LogWarning("Failed to open: {}", path);
        return false;
    }

    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    Json::String errs;
    bool is_parse_successful = parseFromStream(builder, file, &json, &errs);
    if (!is_parse_successful) {
        utility::LogWarning("Read JSON failed: {}.", errs);
        return false;
    }
    return true;
}

std::vector<std::pair<double, Eigen::Matrix4d>> LoadTUMTrajectory(
        const std::string& filename) {
    std::vector<std::pair<double, Eigen::Matrix4d>> trajectory;

    std::ifstream file(filename);
    if (!file.is_open()) {
        utility::LogError("Failed to open: {}", filename);
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        double timestamp, x, y, z, qx, qy, qz, qw;
        if (!(iss >> timestamp >> x >> y >> z >> qx >> qy >> qz >> qw)) {
            utility::LogError("Error parsing line: {}", line);
        }

        Eigen::Affine3d transform = Eigen::Affine3d::Identity();
        transform.translate(Eigen::Vector3d(x, y, z));
        transform.rotate(Eigen::Quaterniond(qw, qx, qy, qz));

        trajectory.emplace_back(timestamp, transform.matrix());
    }

    return trajectory;
}

}  // namespace

const static DataDescriptor data_descriptor = {
        Open3DDownloadsPrefix() +
                "doppler-icp-data/carla-town05-curved-walls.zip",
        "73a9828fb7790481168124c02398ee01"};

DemoDopplerICPSequence::DemoDopplerICPSequence(const std::string& data_root)
    : DownloadDataset("DemoDopplerICPSequence", data_descriptor, data_root) {
    for (int i = 1; i <= 100; ++i) {
        std::stringstream ss;
        ss << std::setw(5) << std::setfill('0') << i;
        paths_.push_back(GetExtractDir() + "/xyzd_sequence/" + ss.str() +
                         ".xyzd");
    }

    calibration_path_ = GetExtractDir() + "/calibration.json";
    trajectory_path_ = GetExtractDir() + "/ground_truth_poses.txt";
}

std::string DemoDopplerICPSequence::GetPath(std::size_t index) const {
    if (index > 99) {
        utility::LogError(
                "Invalid index. Expected index between 0 to 99 but got {}.",
                index);
    }
    return paths_[index];
}

bool DemoDopplerICPSequence::GetCalibration(Eigen::Matrix4d& calibration,
                                            double& period) const {
    Json::Value calibration_data;
    Eigen::Matrix4d calibration_temp;
    if (ReadJSONFromFile(calibration_path_, calibration_data) &&
        utility::IJsonConvertible::EigenMatrix4dFromJsonArray(
                calibration_temp,
                calibration_data["transform_vehicle_to_sensor"])) {
        calibration = calibration_temp.transpose();
        period = calibration_data["period"].asDouble();
        return true;
    }
    return false;
}

std::vector<std::pair<double, Eigen::Matrix4d>>
DemoDopplerICPSequence::GetTrajectory() const {
    return LoadTUMTrajectory(trajectory_path_);
}

}  // namespace data
}  // namespace open3d
