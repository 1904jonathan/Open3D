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

#include "open3d/core/Tensor.h"
#include "open3d/geometry/TriangleMesh.h"
#include "open3d/t/geometry/Geometry.h"
#include "open3d/t/geometry/TensorMap.h"

namespace open3d {
namespace t {
namespace geometry {

/// \class TriangleMesh
/// \brief A triangle mesh contains vertices and triangles.
///
/// The triangle mesh class stores the attribute data in key-value maps. There
/// are two maps by default: the vertex attributes map, and the triangle
/// attribute map.
///
/// - Default attribute: vertex_attr_["positions"], triangle_attr_["indices"]
///     - Vertex positions
///         - TriangleMesh::GetVertexPositions()
///         - TriangleMesh::SetVertexPositions(vertex_positions)
///         - TriangleMesh::HasVertexPositions()
///         - Value tensor must have shape {num_vertices, 3}.
///     - Triangle indices
///         - TriangleMesh::GetTriangleIndices()
///         - TriangleMesh::SetTriangleIndices(triangle_indices)
///         - TriangleMesh::HasTriangleIndices()
///         - Value tensor must have shape {num_triangles, 3}.
///     - Created by default, required for all triangle meshes.
///     - The device of vertex positions and triangle indices must be the same.
///       They determine the device of the trianglemesh.
///
/// - Common attributes: vertex_attr_["normals"], vertex_attr_["colors"]
///                      triangle_attr_["normals"], triangle_attr_["colors"]
///     - Vertex normals
///         - TriangleMesh::GetVertexNormals()
///         - TriangleMesh::SetVertexNormals(vertex_normals)
///         - TriangleMesh::HasVertexNormals()
///         - Value tensor must have shape {num_vertices, 3}.
///         - Value tensor can have any dtype.
///     - Vertex colors
///         - TriangleMesh::GetVertexColors()
///         - TriangleMesh::SetVertexColors(vertex_colors)
///         - TriangleMesh::HasVertexColors()
///         - Value tensor must have shape {num_vertices, 3}.
///         - Value tensor can have any dtype.
///     - Triangle normals
///         - TriangleMesh::GetTriangleNormals()
///         - TriangleMesh::SetTriangleNormals(triangle_normals)
///         - TriangleMesh::HasTriangleNormals()
///         - Value tensor must have shape {num_triangles, 3}.
///         - Value tensor can have any dtype.
///     - Triangle colors
///         - TriangleMesh::GetTriangleColors()
///         - TriangleMesh::SetTriangleColors(triangle_colors)
///         - TriangleMesh::HasTriangleColors()
///         - Value tensor must have shape {num_triangles, 3}.
///         - Value tensor can have any dtype.
///     - Not created by default.
///     - For all attributes above, the device must be consistent with the
///       device of the triangle mesh.
///
/// - Custom attributes: e.g. vetex_attr_["labels"], triangle_attr_["labels"]
///     - Use generalized helper functions, e.g.:
///         - TriangleMesh::GetVertexAttr("labels")
///         - TriangleMesh::SetVertexAttr("labels", vertex_labels)
///         - TriangleMesh::HasVertexAttr("labels")
///         - TriangleMesh::GetTriangleAttr("labels")
///         - TriangleMesh::SetTriangleAttr("labels", triangle_labels)
///         - TriangleMesh::HasTriangleAttr("labels")
///     - Not created by default. Users and add their own custom attributes.
///     - Value tensor must be on the same device as the triangle mesh.
///
/// Note that the level 0 and level 1 convenience functions can also be achieved
/// via the generalized helper functions.
class TriangleMesh : public Geometry {
public:
    /// Construct an empty trianglemesh.
    TriangleMesh(const core::Device &device = core::Device("CPU:0"));

    /// Construct a trianglemesh from vertices and triangles.
    ///
    /// The input tensors will be directly used as the underlying storage of
    /// the trianglemsh (no memory copy). If the tensor is created in-place
    /// from a pre-allocated buffer, the tensor has a fixed size and thus
    /// the resulting trianglemesh will have a fixed size and calling to
    /// functions like `SynchronizedPushBack` will raise an exception.
    ///
    /// The resulting trianglemesh will have the same dtype and device as the
    /// tensor. The device for \p vertex_positions must be consistent with
    /// \p triangle_indices.
    ///
    /// \param vertex_positions A tensor with element shape (3,).
    /// \param triangle_indices A tensor with element shape (3,).
    TriangleMesh(const core::Tensor &vertex_positions,
                 const core::Tensor &triangle_indices);

    virtual ~TriangleMesh() override {}

public:
    /// Transfer the triangle mesh to a specified device.
    /// \param device The targeted device to convert to.
    /// \param copy If true, a new triangle mesh is always created; if false,
    /// the copy is avoided when the original triangle mesh is already on the
    /// targeted device.
    TriangleMesh To(const core::Device &device, bool copy = false) const;

    /// Returns copy of the triangle mesh on the same device.
    TriangleMesh Clone() const { return To(GetDevice()); }

    /// Transfer the triangle mesh to CPU.
    ///
    /// If the triangle mesh is already on CPU, no copy will be performed.
    TriangleMesh CPU() const { return To(core::Device("CPU:0")); };

    /// Transfer the triangle mesh to a CUDA device.
    ///
    /// If the triangle mesh is already on the specified CUDA device, no copy
    /// will be performed.
    TriangleMesh CUDA(int device_id = 0) const {
        return To(core::Device(core::Device::DeviceType::CUDA, device_id));
    };

    /// Getter for vertex_attr_ TensorMap. Used in Pybind.
    const TensorMap &GetVertexAttr() const { return vertex_attr_; }

    /// Get vertex attributes in vertex_attr_. Throws exception if the attribute
    /// does not exist.
    ///
    /// \param key Attribute name.
    core::Tensor &GetVertexAttr(const std::string &key) {
        return vertex_attr_.at(key);
    }

    /// Get the value of the "positions" attribute in vertex_attr_.
    /// Convenience function.
    core::Tensor &GetVertexPositions() { return GetVertexAttr("positions"); }

    /// Get the value of the "colors" attribute in vertex_attr_.
    /// Convenience function.
    core::Tensor &GetVertexColors() { return GetVertexAttr("colors"); }

    /// Get the value of the "normals" attribute in vertex_attr_.
    /// Convenience function.
    core::Tensor &GetVertexNormals() { return GetVertexAttr("normals"); }

    /// Getter for triangle_attr_ TensorMap. Used in Pybind.
    const TensorMap &GetTriangleAttr() const { return triangle_attr_; }

    /// Get triangle attributes in triangle_attr_. Throws exception if the
    /// attribute does not exist.
    ///
    /// \param key Attribute name.
    core::Tensor &GetTriangleAttr(const std::string &key) {
        return triangle_attr_.at(key);
    }

    /// Get the value of the "indices" attribute in triangle_attr_.
    /// Convenience function.
    core::Tensor &GetTriangleIndices() { return GetTriangleAttr("indices"); }

    /// Get the value of the "normals" attribute in triangle_attr_.
    /// Convenience function.
    core::Tensor &GetTriangleNormals() { return GetTriangleAttr("normals"); }

    /// Get the value of the "colors" attribute in triangle_attr_.
    /// Convenience function.
    core::Tensor &GetTriangleColors() { return GetTriangleAttr("colors"); }

    /// Get vertex attributes. Throws exception if the attribute does not exist.
    ///
    /// \param key Attribute name.
    const core::Tensor &GetVertexAttr(const std::string &key) const {
        return vertex_attr_.at(key);
    }

    /// Removes vertex attribute by key value. Primary attribute "positions"
    /// cannot be removed. Throws warning if attribute key does not exists.
    ///
    /// \param key Attribute name.
    void RemoveVertexAttr(const std::string &key) { vertex_attr_.Erase(key); }

    /// Get the value of the "positions" attribute in vertex_attr_.
    /// Convenience function.
    const core::Tensor &GetVertexPositions() const {
        return GetVertexAttr("positions");
    }

    /// Get the value of the "colors" attribute in vertex_attr_.
    /// Convenience function.
    const core::Tensor &GetVertexColors() const {
        return GetVertexAttr("colors");
    }

    /// Get the value of the "normals" attribute in vertex_attr_.
    /// Convenience function.
    const core::Tensor &GetVertexNormals() const {
        return GetVertexAttr("normals");
    }

    /// Get triangle attributes in triangle_attr_. Throws exception if the
    /// attribute does not exist.
    ///
    /// \param key Attribute name.
    const core::Tensor &GetTriangleAttr(const std::string &key) const {
        return triangle_attr_.at(key);
    }

    /// Removes triangle attribute by key value. Primary attribute "indices"
    /// cannot be removed. Throws warning if attribute key does not exists.
    ///
    /// \param key Attribute name.
    void RemoveTriangleAttr(const std::string &key) {
        triangle_attr_.Erase(key);
    }

    /// Get the value of the "indices" attribute in triangle_attr_.
    /// Convenience function.
    const core::Tensor &GetTriangleIndices() const {
        return GetTriangleAttr("indices");
    }

    /// Get the value of the "normals" attribute in triangle_attr_.
    /// Convenience function.
    const core::Tensor &GetTriangleNormals() const {
        return GetTriangleAttr("normals");
    }

    /// Get the value of the "colors" attribute in triangle_attr_.
    /// Convenience function.
    const core::Tensor &GetTriangleColors() const {
        return GetTriangleAttr("colors");
    }

    /// Set vertex attributes. If the attribute key already exists, its value
    /// will be overwritten, otherwise, the new key will be created.
    ///
    /// \param key Attribute name.
    /// \param value A tensor.
    void SetVertexAttr(const std::string &key, const core::Tensor &value) {
        value.AssertDevice(device_);
        vertex_attr_[key] = value;
    }

    /// Set the value of the "positions" attribute in vertex_attr_.
    /// Convenience function.
    void SetVertexPositions(const core::Tensor &value) {
        value.AssertShapeCompatible({utility::nullopt, 3});
        SetVertexAttr("positions", value);
    }

    /// Set the value of the "colors" attribute in vertex_attr_.
    /// Convenience function.
    void SetVertexColors(const core::Tensor &value) {
        value.AssertShapeCompatible({utility::nullopt, 3});
        SetVertexAttr("colors", value);
    }

    /// Set the value of the "normals" attribute in vertex_attr_.
    /// This is a convenience function.
    void SetVertexNormals(const core::Tensor &value) {
        value.AssertShapeCompatible({utility::nullopt, 3});
        SetVertexAttr("normals", value);
    }

    /// Set triangle attributes. If the attribute key already exists, its value
    /// will be overwritten, otherwise, the new key will be created.
    ///
    /// \param key Attribute name.
    /// \param value A tensor.
    void SetTriangleAttr(const std::string &key, const core::Tensor &value) {
        value.AssertDevice(device_);
        triangle_attr_[key] = value;
    }

    /// Set the vlaue of the "indices" attribute in triangle_attr_.
    void SetTriangleIndices(const core::Tensor &value) {
        value.AssertShapeCompatible({utility::nullopt, 3});
        SetTriangleAttr("indices", value);
    }

    /// Set the value of the "normals" attribute in triangle_attr_.
    /// This is a convenience function.
    void SetTriangleNormals(const core::Tensor &value) {
        value.AssertShapeCompatible({utility::nullopt, 3});
        SetTriangleAttr("normals", value);
    }

    /// Set the value of the "colors" attribute in triangle_attr_.
    /// This is a convenience function.
    void SetTriangleColors(const core::Tensor &value) {
        value.AssertShapeCompatible({utility::nullopt, 3});
        SetTriangleAttr("colors", value);
    }

    /// Returns true if all of the followings are true in vertex_attr_:
    /// 1) attribute key exist
    /// 2) attribute's length as vertices' length
    /// 3) attribute's length > 0
    bool HasVertexAttr(const std::string &key) const {
        return vertex_attr_.Contains(key) &&
               GetVertexAttr(key).GetLength() > 0 &&
               GetVertexAttr(key).GetLength() ==
                       GetVertexPositions().GetLength();
    }

    /// Check if the "positions" attribute's value in vertex_attr_ has length >
    /// 0. Convenience function.
    bool HasVertexPositions() const { return HasVertexAttr("positions"); }

    /// Returns true if all of the followings are true in vertex_attr_:
    /// 1) attribute "colors" exist
    /// 2) attribute "colors"'s length as vertices' length
    /// 3) attribute "colors"'s length > 0
    /// Convenience function.
    bool HasVertexColors() const { return HasVertexAttr("colors"); }

    /// Returns true if all of the followings are true in vertex_attr_:
    /// 1) attribute "normals" exist
    /// 2) attribute "normals"'s length as vertices' length
    /// 3) attribute "normals"'s length > 0
    /// Convenience function.
    bool HasVertexNormals() const { return HasVertexAttr("normals"); }

    /// Returns true if all of the followings are true in triangle_attr_:
    /// 1) attribute key exist
    /// 2) attribute's length as triangles' length
    /// 3) attribute's length > 0
    bool HasTriangleAttr(const std::string &key) const {
        return triangle_attr_.Contains(key) &&
               GetTriangleAttr(key).GetLength() > 0 &&
               GetTriangleAttr(key).GetLength() ==
                       GetTriangleIndices().GetLength();
    }

    /// Check if the "indices" attribute's value in triangle_attr_ has length
    /// > 0.
    /// Convenience function.
    bool HasTriangleIndices() const { return HasTriangleAttr("indices"); }

    /// Returns true if all of the followings are true in triangle_attr_:
    /// 1) attribute "normals" exist
    /// 2) attribute "normals"'s length as vertices' length
    /// 3) attribute "normals"'s length > 0
    /// Convenience function.
    bool HasTriangleNormals() const { return HasTriangleAttr("normals"); }

    /// Returns true if all of the followings are true in triangle_attr_:
    /// 1) attribute "colors" exist
    /// 2) attribute "colors"'s length as vertices' length
    /// 3) attribute "colors"'s length > 0
    /// Convenience function.
    bool HasTriangleColors() const { return HasTriangleAttr("colors"); }

public:
    /// Clear all data in the trianglemesh.
    TriangleMesh &Clear() override {
        vertex_attr_.clear();
        triangle_attr_.clear();
        return *this;
    }

    /// Returns !HasVertexPositions(), triangles are ignored.
    bool IsEmpty() const override { return !HasVertexPositions(); }

    core::Tensor GetMinBound() const { return GetVertexPositions().Min({0}); }

    core::Tensor GetMaxBound() const { return GetVertexPositions().Max({0}); }

    core::Tensor GetCenter() const { return GetVertexPositions().Mean({0}); }

    TriangleMesh &Transform(const core::Tensor &transformation);

    TriangleMesh &Translate(const core::Tensor &translation,
                            bool relative = true);

    TriangleMesh &Scale(double scale, const core::Tensor &center);

    TriangleMesh &Rotate(const core::Tensor &R, const core::Tensor &center);

    core::Device GetDevice() const { return device_; }

    /// Create a TriangleMesh from a legacy Open3D TriangleMesh.
    /// \param mesh_legacy Legacy Open3D TriangleMesh.
    /// \param float_dtype Float32 or Float64, used to store floating point
    /// values, e.g. vertices, normals, colors.
    /// \param int_dtype Int32 or Int64, used to store index values, e.g.
    /// triangles.
    /// \param device The device where the resulting TriangleMesh resides in.
    static geometry::TriangleMesh FromLegacy(
            const open3d::geometry::TriangleMesh &mesh_legacy,
            core::Dtype float_dtype = core::Float32,
            core::Dtype int_dtype = core::Int64,
            const core::Device &device = core::Device("CPU:0"));

    /// Convert to a legacy Open3D TriangleMesh.
    open3d::geometry::TriangleMesh ToLegacy() const;

protected:
    core::Device device_ = core::Device("CPU:0");
    TensorMap vertex_attr_;
    TensorMap triangle_attr_;
};

}  // namespace geometry
}  // namespace t
}  // namespace open3d
