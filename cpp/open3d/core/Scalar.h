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

#pragma once

#include <cstdint>
#include <string>

#include "open3d/core/Dtype.h"
#include "open3d/utility/Console.h"

namespace open3d {
namespace core {

/// Scalar is a stores one of {double, int64, bool}. Typically Scalar is used to
/// accept C++ scalar arguments of different types via implicit conversion
/// constructor. Doing so can avoid the need for templates.
class Scalar {
public:
    enum class ScalarType { Double, Int64, Bool };

    Scalar(float v) {
        scalar_mode_ = ScalarType::Double;
        value_.d = static_cast<double>(v);
    }
    Scalar(double v) {
        scalar_mode_ = ScalarType::Double;
        value_.d = static_cast<double>(v);
    }
    Scalar(int v) {
        scalar_mode_ = ScalarType::Int64;
        value_.i = static_cast<int64_t>(v);
    }
    Scalar(int64_t v) {
        scalar_mode_ = ScalarType::Int64;
        value_.i = static_cast<int64_t>(v);
    }
    Scalar(uint8_t v) {
        scalar_mode_ = ScalarType::Int64;
        value_.i = static_cast<int64_t>(v);
    }
    Scalar(uint16_t v) {
        scalar_mode_ = ScalarType::Int64;
        value_.i = static_cast<int64_t>(v);
    }
    Scalar(bool v) {
        scalar_mode_ = ScalarType::Bool;
        value_.b = static_cast<bool>(v);
    }

    bool IsDouble() const { return scalar_mode_ == ScalarType::Double; }
    bool IsInt64() const { return scalar_mode_ == ScalarType::Int64; }
    bool IsBool() const { return scalar_mode_ == ScalarType::Bool; }

    double ToDouble() const {
        if (!IsDouble()) {
            utility::LogError("Scalar is not a ScalarType:Double type.");
        }
        return value_.d;
    }
    double ToInt64() const {
        if (!IsInt64()) {
            utility::LogError("Scalar is not a ScalarType:Int64 type.");
        }
        return value_.i;
    }
    double ToBool() const {
        if (!IsBool()) {
            utility::LogError("Scalar is not a ScalarType:Bool type.");
        }
        return value_.b;
    }

    void AssertSameScalarType(Scalar other,
                              const std::string& error_msg) const {
        if (scalar_mode_ != other.scalar_mode_) {
            if (error_msg.empty()) {
                utility::LogError("Scalar mode {} are not the same as {}.",
                                  ToString(), other.ToString());
            } else {
                utility::LogError("Scalar mode {} are not the same as {}: {}",
                                  ToString(), other.ToString(), error_msg);
            }
        }
    }

    std::string ToString() const {
        std::string scalar_mode_str;
        std::string value_str;
        if (scalar_mode_ == ScalarType::Double) {
            scalar_mode_str = "Double";
            value_str = std::to_string(value_.d);
        } else if (scalar_mode_ == ScalarType::Int64) {
            scalar_mode_str = "Int64";
            value_str = std::to_string(value_.i);
        } else if (scalar_mode_ == ScalarType::Bool) {
            scalar_mode_str = "Bool";
            value_str = value_.b ? "true" : "false";
        } else {
            utility::LogError("ScalarTypeToString: ScalarType not supported.");
        }
        return scalar_mode_str + ":" + value_str;
    }

    template <typename T>
    bool Equal(T value) const {
        if (scalar_mode_ == ScalarType::Double) {
            return value_.d == value;
        } else if (scalar_mode_ == ScalarType::Int64) {
            return value_.i == value;
        } else if (scalar_mode_ == ScalarType::Bool) {
            return false;  // Boolean does not equal to non-boolean values.
        } else {
            utility::LogError("Equals: ScalarType not supported.");
        }
    }

    bool Equal(bool value) const {
        return scalar_mode_ == ScalarType::Bool && value_.b == value;
    }

    bool Equal(Scalar other) const {
        if (other.scalar_mode_ == ScalarType::Double) {
            return Equal(other.ToDouble());
        } else if (other.scalar_mode_ == ScalarType::Int64) {
            return Equal(other.ToInt64());
        } else if (other.scalar_mode_ == ScalarType::Bool) {
            return scalar_mode_ == ScalarType::Bool &&
                   value_.b == other.value_.b;
        } else {
            utility::LogError("Equals: ScalarType not supported.");
        }
    }

private:
    ScalarType scalar_mode_;
    union value_t {
        double d;
        int64_t i;
        bool b;
    } value_;
};

}  // namespace core
}  // namespace open3d
