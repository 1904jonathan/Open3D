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

#include "ModelInteractor.h"

#include "Open3D/Visualization/Rendering/Scene.h"

namespace open3d {
namespace visualization {

ModelInteractor::ModelInteractor(visualization::Scene* scene,
                                 visualization::Camera* camera,
                                 double minFarPlane)
    : RotationInteractor(camera, minFarPlane),
      scene_(scene),
      isAxesVisible_(false) {}

ModelInteractor::~ModelInteractor() {}

void ModelInteractor::SetBoundingBox(
        const geometry::AxisAlignedBoundingBox& bounds) {
    Super::SetBoundingBox(bounds);
    // Initialize parent's matrix_ (in case we do a mouse wheel, which
    // doesn't involve a mouse down) and the center of rotation.
    SetMouseDownInfo(visualization::Camera::Transform::Identity(),
                     bounds.GetCenter().cast<float>());
}

void ModelInteractor::SetModel(GeometryHandle axes,
                               const std::vector<GeometryHandle>& objects) {
    axes_ = axes;
    model_ = objects;
}

void ModelInteractor::Rotate(int dx, int dy) {
    Eigen::Vector3f xAxis = -camera_->GetLeftVector();
    Eigen::Vector3f yAxis = camera_->GetUpVector();

    Eigen::Vector3f axis = -dy * xAxis + dx * yAxis;
    axis = axis.normalized();
    float theta = CalcRotateRadians(dx, dy);
    Eigen::AngleAxisf rotMatrix(theta, axis);

    // Rotations about a point using a world axis do not produce
    // a matrix that can be applied to any matrix; each individual
    // matrix must be rotated around the point.
    for (auto o : model_) {
        Camera::Transform t = transformsAtMouseDown_[o];  // copy
        t.translate(centerOfRotation_);
        t.fromPositionOrientationScale(t.translation(),
                                       rotMatrix * t.rotation(),
                                       Eigen::Vector3f(1, 1, 1));
        t.translate(-centerOfRotation_);
        scene_->SetEntityTransform(o, t);
    }
    UpdateBoundingBox(Camera::Transform(rotMatrix));
}

void ModelInteractor::RotateZ(int dx, int dy) {
    auto rad = CalcRotateZRadians(dx, dy);
    Eigen::AngleAxisf rotMatrix(rad, camera_->GetForwardVector());

    for (auto o : model_) {
        Camera::Transform t = transformsAtMouseDown_[o];  // copy
        t.translate(centerOfRotation_);
        t.fromPositionOrientationScale(t.translation(),
                                       rotMatrix * t.rotation(),
                                       Eigen::Vector3f(1, 1, 1));
        t.translate(-centerOfRotation_);
        scene_->SetEntityTransform(o, t);
    }
    UpdateBoundingBox(Camera::Transform(rotMatrix));
}

void ModelInteractor::Dolly(int dy, DragType dragType) {
    float zDist = CalcDollyDist(dy, dragType);
    Eigen::Vector3f worldMove = -zDist * camera_->GetForwardVector();

    for (auto o : model_) {
        Camera::Transform t;
        if (dragType == DragType::MOUSE) {
            t = transformsAtMouseDown_[o];  // copy
        } else {
            t = scene_->GetEntityTransform(o);
        }
        Eigen::Vector3f newTrans = t.translation() + worldMove;
        t.fromPositionOrientationScale(newTrans, t.rotation(),
                                       Eigen::Vector3f(1, 1, 1));
        scene_->SetEntityTransform(o, t);
    }

    Camera::Transform t = Camera::Transform::Identity();
    t.translate(worldMove);
    UpdateBoundingBox(t);

    UpdateCameraFarPlane();
}

void ModelInteractor::Pan(int dx, int dy) {
    Eigen::Vector3f worldMove = CalcPanVectorWorld(-dx, -dy);
    centerOfRotation_ = centerOfRotationAtMouseDown_ + worldMove;

    for (auto o : model_) {
        Camera::Transform t = transformsAtMouseDown_[o];  // copy
        Eigen::Vector3f newTrans = t.translation() + worldMove;
        t.fromPositionOrientationScale(newTrans, t.rotation(),
                                       Eigen::Vector3f(1, 1, 1));
        scene_->SetEntityTransform(o, t);
    }
    Camera::Transform t = Camera::Transform::Identity();
    t.translate(worldMove);
    UpdateBoundingBox(t);
}

void ModelInteractor::UpdateBoundingBox(const Camera::Transform& t) {
    using Transform4 = Eigen::Transform<double, 3, Eigen::Affine>;
    Transform4 change = t.cast<double>();
    Eigen::Vector3d newMin = change * modelBounds_.GetMinBound();
    Eigen::Vector3d newMax = change * modelBounds_.GetMaxBound();
    // Call super's not our SetBoundingBox(): we also update the
    // center of rotation, because normally this call is not done during
    // mouse movement, but rather, once to initalize the interactor.
    Super::SetBoundingBox(geometry::AxisAlignedBoundingBox(newMin, newMax));
}

void ModelInteractor::StartMouseDrag() {
    SetMouseDownInfo(Camera::Transform::Identity(), centerOfRotation_);

    transformsAtMouseDown_.clear();
    for (auto o : model_) {
        transformsAtMouseDown_[o] = scene_->GetEntityTransform(o);
    }

    // Show axes while user is dragging
    isAxesVisible_ = scene_->GetEntityEnabled(axes_);
    scene_->SetEntityEnabled(axes_, true);
}

void ModelInteractor::UpdateMouseDragUI() {}

void ModelInteractor::EndMouseDrag() {
    scene_->SetEntityEnabled(axes_, isAxesVisible_);
}

}  // namespace visualization
}  // namespace open3d
