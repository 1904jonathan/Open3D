// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2019 www.open3d.org
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

#include "open3d/visualization/rendering/filament/FilamentCamera.h"

// 4068: Filament has some clang-specific vectorizing pragma's that MSVC flags
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4068)
#endif  // _MSC_VER

#include <filament/Camera.h>
#include <filament/Engine.h>
#include <math/mat4.h>  // necessary for mat4f

#ifdef _MSC_VER
#pragma warning(pop)
#endif  // _MSC_VER

namespace open3d {
namespace visualization {
namespace rendering {

namespace {
Camera::Transform FilamentToCameraTransform(const filament::math::mat4& ft) {
    Camera::Transform::MatrixType m;

    m << float(ft(0, 0)), float(ft(0, 1)), float(ft(0, 2)), float(ft(0, 3)),
            float(ft(1, 0)), float(ft(1, 1)), float(ft(1, 2)), float(ft(1, 3)),
            float(ft(2, 0)), float(ft(2, 1)), float(ft(2, 2)), float(ft(2, 3)),
            float(ft(3, 0)), float(ft(3, 1)), float(ft(3, 2)), float(ft(3, 3));

    return Camera::Transform(m);
}

Camera::Transform FilamentToCameraTransform(const filament::math::mat4f& ft) {
    Camera::Transform::MatrixType m;

    m << ft(0, 0), ft(0, 1), ft(0, 2), ft(0, 3), ft(1, 0), ft(1, 1), ft(1, 2),
            ft(1, 3), ft(2, 0), ft(2, 1), ft(2, 2), ft(2, 3), ft(3, 0),
            ft(3, 1), ft(3, 2), ft(3, 3);

    return Camera::Transform(m);
}

filament::math::mat4f CameraToFilamentTransformF(const Camera::Transform& t) {
    auto e_matrix = t.matrix();
    return filament::math::mat4f(filament::math::mat4f::row_major_init{
            e_matrix(0, 0), e_matrix(0, 1), e_matrix(0, 2), e_matrix(0, 3),
            e_matrix(1, 0), e_matrix(1, 1), e_matrix(1, 2), e_matrix(1, 3),
            e_matrix(2, 0), e_matrix(2, 1), e_matrix(2, 2), e_matrix(2, 3),
            e_matrix(3, 0), e_matrix(3, 1), e_matrix(3, 2), e_matrix(3, 3)});
}

}  // namespace

FilamentCamera::FilamentCamera(filament::Engine& engine) : engine_(engine) {
    camera_ = engine_.createCamera();
}

FilamentCamera::~FilamentCamera() { engine_.destroy(camera_); }

void FilamentCamera::CopyFrom(const Camera* camera) {
    SetModelMatrix(camera->GetModelMatrix());

    auto& proj = camera->GetProjection();
    if (proj.is_ortho) {
        SetProjection(proj.proj.ortho.projection, proj.proj.ortho.left,
                      proj.proj.ortho.right, proj.proj.ortho.bottom,
                      proj.proj.ortho.top, proj.proj.ortho.near_plane,
                      proj.proj.ortho.far_plane);
    } else {
        SetProjection(proj.proj.perspective.fov, proj.proj.perspective.aspect,
                      proj.proj.perspective.near_plane,
                      proj.proj.perspective.far_plane,
                      proj.proj.perspective.fov_type);
    }
}

void FilamentCamera::SetProjection(
        double fov, double aspect, double near, double far, FovType fov_type) {
    if (aspect > 0.0) {
        filament::Camera::Fov dir = (fov_type == FovType::Horizontal)
                                            ? filament::Camera::Fov::HORIZONTAL
                                            : filament::Camera::Fov::VERTICAL;

        camera_->setProjection(fov, aspect, near, far, dir);

        projection_.is_ortho = false;
        projection_.proj.perspective.fov_type = fov_type;
        projection_.proj.perspective.fov = fov;
        projection_.proj.perspective.aspect = aspect;
        projection_.proj.perspective.near_plane = near;
        projection_.proj.perspective.far_plane = far;
    }
}

void FilamentCamera::SetProjection(Projection projection,
                                   double left,
                                   double right,
                                   double bottom,
                                   double top,
                                   double near,
                                   double far) {
    filament::Camera::Projection proj =
            (projection == Projection::Ortho)
                    ? filament::Camera::Projection::ORTHO
                    : filament::Camera::Projection::PERSPECTIVE;

    camera_->setProjection(proj, left, right, bottom, top, near, far);

    projection_.is_ortho = true;
    projection_.proj.ortho.projection = projection;
    projection_.proj.ortho.left = left;
    projection_.proj.ortho.right = right;
    projection_.proj.ortho.bottom = bottom;
    projection_.proj.ortho.top = top;
    projection_.proj.ortho.near_plane = near;
    projection_.proj.ortho.far_plane = far;
}

double FilamentCamera::GetNear() const { return camera_->getNear(); }

double FilamentCamera::GetFar() const { return camera_->getCullingFar(); }

double FilamentCamera::GetFieldOfView() const {
    if (projection_.is_ortho) {
        // technically orthographic projection is lim(fov->0) as dist->inf,
        // but it also serves as an obviously wrong value if you call
        // GetFieldOfView() after setting an orthographic projection
        return 0.0;
    } else {
        return projection_.proj.perspective.fov;
    }
}

Camera::FovType FilamentCamera::GetFieldOfViewType() const {
    return projection_.proj.perspective.fov_type;
}

void FilamentCamera::LookAt(const Eigen::Vector3f& center,
                            const Eigen::Vector3f& eye,
                            const Eigen::Vector3f& up) {
    camera_->lookAt({eye.x(), eye.y(), eye.z()},
                    {center.x(), center.y(), center.z()},
                    {up.x(), up.y(), up.z()});
}

Eigen::Vector3f FilamentCamera::GetPosition() const {
    auto cam_pos = camera_->getPosition();
    return {cam_pos.x, cam_pos.y, cam_pos.z};
}

Eigen::Vector3f FilamentCamera::GetForwardVector() const {
    auto forward = camera_->getForwardVector();
    return {forward.x, forward.y, forward.z};
}

Eigen::Vector3f FilamentCamera::GetLeftVector() const {
    auto left = camera_->getLeftVector();
    return {left.x, left.y, left.z};
}

Eigen::Vector3f FilamentCamera::GetUpVector() const {
    auto up = camera_->getUpVector();
    return {up.x, up.y, up.z};
}

Camera::Transform FilamentCamera::GetModelMatrix() const {
    auto ftransform = camera_->getModelMatrix();
    return FilamentToCameraTransform(ftransform);
}

Camera::Transform FilamentCamera::GetViewMatrix() const {
    auto ftransform = camera_->getViewMatrix();  // returns mat4 (not mat4f)
    return FilamentToCameraTransform(ftransform);
}

Camera::Transform FilamentCamera::GetProjectionMatrix() const {
    auto ftransform = camera_->getProjectionMatrix();  // mat4 (not mat4f)
    return FilamentToCameraTransform(ftransform);
}

const Camera::ProjectionInfo& FilamentCamera::GetProjection() const {
    return projection_;
}

void FilamentCamera::SetModelMatrix(const Eigen::Vector3f& forward,
                                    const Eigen::Vector3f& left,
                                    const Eigen::Vector3f& up) {
    using namespace filament;

    math::mat4f ftransform = camera_->getModelMatrix();
    ftransform[0].xyz = math::float3(left.x(), left.y(), left.z());
    ftransform[1].xyz = math::float3(up.x(), up.y(), up.z());
    ftransform[2].xyz = math::float3(forward.x(), forward.y(), forward.z());

    camera_->setModelMatrix(ftransform);  // model matrix uses mat4f
}

void FilamentCamera::SetModelMatrix(const Transform& view) {
    auto ftransform = CameraToFilamentTransformF(view);
    camera_->setModelMatrix(ftransform);  // model matrix uses mat4f
}

}  // namespace rendering
}  // namespace visualization
}  // namespace open3d
