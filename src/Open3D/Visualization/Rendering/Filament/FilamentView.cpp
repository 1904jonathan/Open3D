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

#include "FilamentView.h"

#include "FilamentCamera.h"
#include "FilamentEntitiesMods.h"
#include "FilamentResourceManager.h"
#include "FilamentScene.h"

#include <filament/Camera.h>
#include <filament/Engine.h>
#include <filament/RenderableManager.h>
#include <filament/Scene.h>
#include <filament/View.h>

namespace open3d {
namespace visualization {

namespace {
const filament::LinearColorA kDepthClearColor = {0.f, 0.f, 0.f, 0.f};
const filament::LinearColorA kNormalsClearColor = {0.5f, 0.5f, 0.5f, 1.f};

filament::View::TargetBufferFlags FlagsFromTargetBuffers(
        const View::TargetBuffers& buffers) {
    using namespace std;

    auto rawBuffers = static_cast<uint8_t>(buffers);
    uint8_t rawFilamentBuffers = 0;
    if (rawBuffers | (uint8_t)View::TargetBuffers::Color) {
        rawFilamentBuffers |= (uint8_t)filament::View::TargetBufferFlags::COLOR;
    }
    if (rawBuffers | (uint8_t)View::TargetBuffers::Depth) {
        rawFilamentBuffers |= (uint8_t)filament::View::TargetBufferFlags::DEPTH;
    }
    if (rawBuffers | (uint8_t)View::TargetBuffers::Stencil) {
        rawFilamentBuffers |=
                (uint8_t)filament::View::TargetBufferFlags::STENCIL;
    }

    return static_cast<filament::View::TargetBufferFlags>(rawFilamentBuffers);
}
}  // namespace

FilamentView::FilamentView(filament::Engine& engine,
                           FilamentResourceManager& resourceManager)
    : engine_(engine), resourceManager_(resourceManager) {
    view_ = engine_.createView();
    view_->setSampleCount(8);
    view_->setAntiAliasing(filament::View::AntiAliasing::FXAA);
    view_->setPostProcessingEnabled(true);
    view_->setVisibleLayers(kAllLayersMask, kMainLayer);

    camera_ = std::make_unique<FilamentCamera>(engine_);
    view_->setCamera(camera_->GetNativeCamera());

    camera_->SetProjection(90, 4.f / 3.f, 0.01, 1000,
                           Camera::FovType::Horizontal);

    discardBuffers_ = View::TargetBuffers::All;
}

FilamentView::FilamentView(filament::Engine& engine,
                           FilamentScene& scene,
                           FilamentResourceManager& resourceManager)
    : FilamentView(engine, resourceManager) {
    scene_ = &scene;

    view_->setScene(scene_->GetNativeScene());
}

FilamentView::~FilamentView() {
    view_->setCamera(nullptr);
    view_->setScene(nullptr);

    camera_.reset();
    engine_.destroy(view_);
}

void FilamentView::SetMode(Mode mode) {
    switch (mode) {
        case Mode::Color:
            view_->setVisibleLayers(kAllLayersMask, kMainLayer);
            view_->setClearColor(
                    {clearColor_.x(), clearColor_.y(), clearColor_.z(), 1.f});
            break;
        case Mode::Depth:
            view_->setVisibleLayers(kAllLayersMask, kMainLayer);
            view_->setClearColor(kDepthClearColor);
            break;
        case Mode::Normals:
            view_->setVisibleLayers(kAllLayersMask, kMainLayer);
            view_->setClearColor(kNormalsClearColor);
            break;
    }

    mode_ = mode;
}

void FilamentView::SetDiscardBuffers(const TargetBuffers& buffers) {
    discardBuffers_ = buffers;
    view_->setRenderTarget(nullptr, FlagsFromTargetBuffers(buffers));
}

void FilamentView::SetViewport(std::int32_t x,
                               std::int32_t y,
                               std::uint32_t w,
                               std::uint32_t h) {
    view_->setViewport({x, y, w, h});
}

void FilamentView::SetClearColor(const Eigen::Vector3f& color) {
    clearColor_ = color;

    if (mode_ == Mode::Color) {
        view_->setClearColor({color.x(), color.y(), color.z(), 1.f});
    }
}

Camera* FilamentView::GetCamera() const { return camera_.get(); }

void FilamentView::CopySettingsFrom(const FilamentView& other) {
    SetMode(other.mode_);
    view_->setRenderTarget(nullptr,
                           FlagsFromTargetBuffers(other.discardBuffers_));

    auto vp = other.view_->getViewport();
    SetViewport(0, 0, vp.width, vp.height);

    SetClearColor(other.clearColor_);

    // TODO: Consider moving this code to FilamentCamera
    auto& camera = view_->getCamera();
    auto& otherCamera = other.GetNativeView()->getCamera();

    // TODO: Code below could introduce problems with culling,
    //        because Camera::setCustomProjection method
    //        assigns both culling projection and projection matrices
    //        to the same matrix. Which is good for ORTHO but
    //        makes culling matrix with infinite far plane for PERSPECTIVE
    //        See FCamera::setCustomProjection and FCamera::setProjection
    //        There is no easy way to fix it currently (Filament 1.4.3)
    camera.setCustomProjection(otherCamera.getProjectionMatrix(),
                               otherCamera.getNear(),
                               otherCamera.getCullingFar());
    camera.setModelMatrix(otherCamera.getModelMatrix());
}

void FilamentView::SetScene(FilamentScene& scene) {
    scene_ = &scene;
    view_->setScene(scene_->GetNativeScene());
}

void FilamentView::PreRender() {
    auto& renderableManager = engine_.getRenderableManager();

    MaterialInstanceHandle materialHandle;
    if (mode_ == Mode::Depth) {
        materialHandle = FilamentResourceManager::kDepthMaterial;
        // FIXME: Refresh parameters only then something ACTUALLY changed
        auto matInst =
                resourceManager_.GetMaterialInstance(materialHandle).lock();
        if (matInst) {
            const auto f = camera_->GetNativeCamera()->getCullingFar();
            const auto n = camera_->GetNativeCamera()->getNear();

            FilamentMaterialModifier(matInst, materialHandle)
                    .SetParameter("cameraNear", n)
                    .SetParameter("cameraFar", f)
                    .Finish();
        }
    } else if (mode_ == Mode::Normals) {
        materialHandle = FilamentResourceManager::kNormalsMaterial;
    }

    if (scene_) {
        for (const auto& pair : scene_->entities_) {
            const auto& entity = pair.second;
            if (entity.info.type == EntityType::Geometry) {
                std::weak_ptr<filament::MaterialInstance> matInst;
                if (materialHandle) {
                    matInst = resourceManager_.GetMaterialInstance(
                            materialHandle);
                } else {
                    matInst = resourceManager_.GetMaterialInstance(
                            entity.material);
                }

                filament::RenderableManager::Instance inst =
                        renderableManager.getInstance(entity.info.self);
                renderableManager.setMaterialInstanceAt(inst, 0,
                                                        matInst.lock().get());
            }
        }
    }
}

void FilamentView::PostRender() {
    // For now, we don't need to restore material.
    // One could easily find assigned material in SceneEntity::material

    //    auto& renderableManager = engine_.getRenderableManager();
    //
    //    for (const auto& pair : scene_.entities_) {
    //        const auto& entity = pair.second;
    //        if (entity.type == EntityType::Geometry) {
    //            auto wMaterialInstance =
    //                    resourceManager_.GetMaterialInstance(entity.material);
    //
    //            filament::RenderableManager::Instance inst =
    //                    renderableManager.getInstance(entity.self);
    //            renderableManager.setMaterialInstanceAt(
    //                    inst, 0, wMaterialInstance.lock().get());
    //        }
    //    }
}

}  // namespace visualization
}  // namespace open3d