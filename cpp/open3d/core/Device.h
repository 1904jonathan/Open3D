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
#include <string>
#include <vector>

#include "open3d/utility/Helper.h"
#include "open3d/utility/Logging.h"

namespace open3d {
namespace core {

/// Device context specifying device type and device id.
/// For CPU, there is only one device with id 0
class Device {
public:
    /// Type for device.
    enum class DeviceType { CPU = 0, CUDA = 1 };

    /// Default constructor.
    Device() = default;

    /// Constructor with device specified.
    explicit Device(DeviceType device_type, int device_id);

    /// Constructor from device type string and device id.
    explicit Device(const std::string& device_type, int device_id);

    /// Constructor from string, e.g. "CUDA:0".
    explicit Device(const std::string& type_colon_id);

    bool operator==(const Device& other) const;

    bool operator!=(const Device& other) const;

    bool operator<(const Device& other) const;

    std::string ToString() const;

    DeviceType GetType() const;

    int GetID() const;

protected:
    void AssertCPUDeviceIDIsZero();

    static DeviceType StringToDeviceType(const std::string& type_colon_id);

    static int StringToDeviceId(const std::string& type_colon_id);

protected:
    DeviceType device_type_ = DeviceType::CPU;
    int device_id_ = 0;
};

}  // namespace core
}  // namespace open3d

namespace std {
template <>
struct hash<open3d::core::Device> {
    std::size_t operator()(const open3d::core::Device& device) const {
        return std::hash<std::string>{}(device.ToString());
    }
};
}  // namespace std
