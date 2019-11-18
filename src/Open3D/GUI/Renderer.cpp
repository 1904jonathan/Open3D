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

#include "Renderer.h"

#include "Color.h"
#include "Window.h"

#include <filament/Box.h>
#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/RenderableManager.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>
#include <filament/geometry/SurfaceOrientation.h>

#include <unordered_map>
#include <vector>

#include <iostream> // debugging; remove

using namespace filament;

namespace open3d {
namespace gui {

namespace {

static const int BOGUS_ID = -1;

void freeTempBuffer(void *buffer, size_t, void*) {
    free(buffer);
}

template <typename T>
class PoolBase {
public:
    using Id = int32_t;
    using Iterator = typename std::unordered_map<Id, T>::iterator;
    using ConstIterator = typename std::unordered_map<Id, T>::const_iterator;

    virtual ~PoolBase() {
        // We can't call DestroyItem() in the destructor, since it is virtual
        // and at this point the derived class has been cleaned up.  So the
        // derived class must call DestroyAll(), which DestroyItem() is still
        // valid.
        assert(items_.empty());
    }

    void DestroyAll() {
        for (auto &kv : items_) {
            DestroyItem(kv.second);
        }
        items_.clear();
    }

    virtual void DestroyItem(T& item) = 0;

    Id Add(const T& newItem) {
        currId_ += 1;
        items_[currId_] = newItem;
        return currId_;
    }

    T& Get(Id id) {
        return items_[id];
    }

    void Remove(Id id) {
        if (Has(id)) {
            DestroyItem(Get(id));
            items_.erase(id);
        }
    }

    bool Has(Id id) const {
        return (items_.find(id) != items_.end());
    }

protected:
    int currId_ = 0;
    std::unordered_map<Id, T> items_;
};

template <typename T>
class ObjectPool : public PoolBase<T> {
public:
    virtual ~ObjectPool() {
        this->DestroyAll();
    }

    void DestroyItem(T& item) override {}
};

template <typename T>
class EngineObjectPool : public PoolBase<T> {
public:
    EngineObjectPool(Engine *engine) {
        engine_ = engine;
    }

    ~EngineObjectPool() {
        this->DestroyAll();
    }

    void DestroyItem(T& item) override {
        engine_->destroy(item);
    }

private:
    Engine *engine_;
};

}

// ----------------------------------------------------------------------------
BoundingBox::BoundingBox()
    : xMin(0), xMax(0), yMin(0), yMax(0), zMin(0), zMax(0) {
}

BoundingBox::BoundingBox(float centerX, float centerY, float centerZ, float radius)
    : xMin(centerX - radius), xMax(centerX + radius)
    , yMin(centerY - radius), yMax(centerY + radius)
    , zMin(centerZ - radius), zMax(centerZ + radius) {
}

BoundingBox::BoundingBox(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
    : xMin(xmin), xMax(xmax), yMin(ymin), yMax(ymax), zMin(zmin), zMax(zmax) {
}

// ----------------------------------------------------------------------------
/*struct Transform::Impl {
    math::mat4 matrix;
}

Transform::Transform() {
    std::cout << "TODO: implement Transform()" << std::endl;
}

Transform::Transform(const Transform& t) {
    std::cout << "TODO: implement Transform(const Transform&)" << std::endl;

}

void Transform::Translate(float x, float y, float z) {
    std::cout << "TODO: implement Transform::Translate()" << std::endl;
}

void Transform::Rotate(float axis_x, float axis_y, float axis_z,
                       float degrees) {
    std::cout << "TODO: implement Transform::Rotate()" << std::endl;
}
*/
// ----------------------------------------------------------------------------
// This just holds all the information.  The buffers get deallocated by other
// means to try to avoid ravioli code.
struct Geometry {
    BoundingBox boundingBox;
    Renderer::VertexBufferId vbufferId = BOGUS_ID;
    Renderer::IndexBufferId ibufferId = BOGUS_ID;
};

struct Renderer::Impl
{
    const Window& window;
    filament::Engine *engine;
    filament::Renderer *renderer;
    filament::SwapChain *swapChain = nullptr;
    struct Alloc {
        EngineObjectPool<filament::View*> views;
        EngineObjectPool<filament::Scene*> scenes;
        EngineObjectPool<filament::Camera*> cameras;
        EngineObjectPool<utils::Entity> renderables;
        EngineObjectPool<filament::VertexBuffer*> vbuffers;
        EngineObjectPool<filament::IndexBuffer*> ibuffers;
        EngineObjectPool<filament::MaterialInstance*> materials;
        ObjectPool<Geometry> geometries;

        Alloc(Engine *e)
            : views(e), scenes(e), cameras(e), renderables(e), vbuffers(e), ibuffers(e), materials(e)
        {}
    };
    // Use naked pointer to indicate we are responsible to manage this:
    // Alloc needs to be freed before engine, since the pointers it stores
    // must be destroyed by the engine with engine->destroy(ptr);
    Alloc *alloc;

    Impl(const Window& w) : window(w) {}
};

// On single-threaded platforms, Filament's OpenGL context must be current,
// not SDL's context.
Renderer::Renderer(const Window& window)
    : impl_(new Impl(window))
{
    impl_->engine = Engine::create(filament::Engine::Backend::OPENGL);
    impl_->alloc = new Impl::Alloc(impl_->engine);
    impl_->renderer = impl_->engine->createRenderer();

    UpdateFromDrawable();
}

Renderer::~Renderer()
{
    delete impl_->alloc;
    impl_->engine->destroy(impl_->swapChain);
    impl_->engine->destroy(impl_->renderer);
    Engine::destroy(&impl_->engine);
}

void Renderer::UpdateFromDrawable()
{
    if (impl_->swapChain) {
        impl_->engine->destroy(impl_->swapChain);
    }

    void* nativeDrawable = impl_->window.GetNativeDrawable();
/*
#if defined(__APPLE__)
    void* metalLayer = nullptr;
    if (config.backend == filament::Engine::Backend::METAL) {
        // The swap chain on Metal is a CAMetalLayer.
        nativeDrawable = setUpMetalLayer(nativeDrawable);
    }
#if defined(FILAMENT_DRIVER_SUPPORTS_VULKAN)
    if (config.backend == filament::Engine::Backend::VULKAN) {
        // We request a Metal layer for rendering via MoltenVK.
        setUpMetalLayer(nativeDrawable);
    }
#endif // FILAMENT_DRIVER_SUPPORTS_VULKAN
#endif // __APPLE__
*/

    impl_->swapChain = impl_->engine->createSwapChain(nativeDrawable);
}

bool Renderer::BeginFrame() {
    return impl_->renderer->beginFrame(impl_->swapChain);
}

void Renderer::Render(Renderer::ViewId viewId) {
    if (auto view = (filament::View*)GetViewPointer(viewId)) {
        impl_->renderer->render(view);
    }
}

void Renderer::EndFrame() {
    impl_->renderer->endFrame();
}

Renderer::ViewId Renderer::CreateView() {
    return impl_->alloc->views.Add(impl_->engine->createView());
}

void Renderer::DestroyView(Renderer::ViewId viewId) {
    return impl_->alloc->views.Remove(viewId);
}

Renderer::SceneId Renderer::CreateScene() {
    return impl_->alloc->scenes.Add(impl_->engine->createScene());
}

void Renderer::DestroyScene(Renderer::SceneId sceneId) {
    return impl_->alloc->scenes.Remove(sceneId);
}

Renderer::CameraId Renderer::CreateCamera() {
    return impl_->alloc->cameras.Add(impl_->engine->createCamera());
}

void Renderer::DestroyCamera(Renderer::CameraId cameraId) {
    return impl_->alloc->cameras.Remove(cameraId);
}

Renderer::MaterialId Renderer::CreateMetal(const Color& baseColor,
                                           float metallic, float roughness,
                                           float anisotropy) {
    std::cout << "Renderer::CreateMetal()" << std::endl;
    return BOGUS_ID;
}

Renderer::MaterialId Renderer::CreateNonMetal(const Color& baseColor,
                                              float roughness, float clearCoat,
                                              float clearCoatRoughness) {
    std::cout << "Renderer::CreateNonMetal" << std::endl;
    return BOGUS_ID;
}

//LightId Renderer::CreateLight(...);
//IBLId Renderer::CreateIBL(...);

Renderer::GeometryId Renderer::CreateGeometry(const std::vector<float>& vertices,
                                              const std::vector<float>& normals,
                                              const std::vector<uint32_t>& indices,
                                              const BoundingBox& bbox) {
    auto engine = impl_->engine;

    int nVerts = vertices.size() / 3;

    std::vector<math::quatf> tangents(nVerts);
    auto orientation = geometry::SurfaceOrientation::Builder()
                                    .vertexCount(nVerts)
                                    .normals((math::float3*)normals.data())
                                    .build();
    orientation.getQuats(tangents.data(), nVerts);

    auto geometryId = impl_->alloc->geometries.Add(Geometry());
    auto &g = impl_->alloc->geometries.Get(geometryId);
    g.boundingBox = bbox;

    // Filament doesn't document whether BufferDescriptor copies the data in
    // the pointer you give it, or whether it keeps the pointer.  It seems to
    // put the buffer in a queue and copy the data on another thread a short
    // time from now. This means that we have to keep the data around for a
    // while, so we need to allocate a special pointer for it.
    int nVertBytes = sizeof(*vertices.data()) * vertices.size();
    int nTanBytes = sizeof(*tangents.data()) * tangents.size();
    int nIdxBytes = sizeof(*indices.data()) * indices.size();
    auto *v = (float*)malloc(nVertBytes);
    memcpy(v, vertices.data(), nVertBytes);
    auto *t = (math::quatf*)malloc(nTanBytes);
    memcpy(t, tangents.data(), nTanBytes);
    auto *i = (uint32_t*)malloc(nIdxBytes);
    memcpy(i, indices.data(), nIdxBytes);

    VertexBuffer::Builder vbb;
    vbb.vertexCount(nVerts)
       .bufferCount(2)
       .normalized(VertexAttribute::TANGENTS)
       .attribute(VertexAttribute::POSITION, 0,
                  VertexBuffer::AttributeType::FLOAT3, 0, 0)
       .attribute(VertexAttribute::TANGENTS, 1,
                  VertexBuffer::AttributeType::FLOAT4, 0, 0);
    auto vbuffer = vbb.build(*engine);
    vbuffer->setBufferAt(*engine, 0,
                         VertexBuffer::BufferDescriptor(v, nVertBytes,
                                                        freeTempBuffer,
                                                        nullptr));
    vbuffer->setBufferAt(*engine, 1,
                         VertexBuffer::BufferDescriptor(t, nTanBytes,
                                                        freeTempBuffer,
                                                        nullptr));
    auto idxType = ((sizeof(indices[0]) == 2) ? IndexBuffer::IndexType::USHORT
                                              : IndexBuffer::IndexType::UINT);
    auto ibuffer = IndexBuffer::Builder()
                            .indexCount(indices.size())
                            .bufferType(idxType)
                            .build(*engine);
    ibuffer->setBuffer(*engine,
                       IndexBuffer::BufferDescriptor(i, nIdxBytes,
                                                     freeTempBuffer,
                                                     nullptr));

    g.vbufferId = impl_->alloc->vbuffers.Add(vbuffer);
    g.ibufferId = impl_->alloc->ibuffers.Add(ibuffer);

    return geometryId;
}

Renderer::MeshId Renderer::CreateMesh(GeometryId geometryId,
                                      MaterialId materialId) {
    if (!impl_->alloc->geometries.Has(geometryId)/* ||
        !impl_->alloc->materials.Has(materialId)*/) {
        return BOGUS_ID;
    }
    auto &g = impl_->alloc->geometries.Get(geometryId);
    auto verts = impl_->alloc->vbuffers.Get(g.vbufferId);
    auto indices = impl_->alloc->ibuffers.Get(g.ibufferId);
//    auto m = impl_->alloc->materials.Get(materialId)
    filament::MaterialInstance *m = nullptr;

    auto renderable = utils::EntityManager::get().create();
    auto meshId = impl_->alloc->renderables.Add(renderable);

    RenderableManager::Builder objBuilder(1);
    objBuilder.boundingBox(Box().set(math::float3(g.boundingBox.xMin,
                                                  g.boundingBox.yMin,
                                                  g.boundingBox.zMin),
                                     math::float3(g.boundingBox.xMax,
                                                  g.boundingBox.yMax,
                                                  g.boundingBox.zMax)));
    objBuilder.material(0, m);
    objBuilder.geometry(0, RenderableManager::PrimitiveType::TRIANGLES,
                        verts, indices);
    objBuilder.build(*impl_->engine, renderable);

    return meshId;
}

void* Renderer::GetViewPointer(ViewId id) {
    if (impl_->alloc->views.Has(id)) {
        return impl_->alloc->views.Get(id);
    } else {
        return nullptr;
    }
}

void* Renderer::GetScenePointer(SceneId id) {
    if (impl_->alloc->scenes.Has(id)) {
        return impl_->alloc->scenes.Get(id);
    } else {
        return nullptr;
    }
}

void* Renderer::GetCameraPointer(CameraId id) {
    if (impl_->alloc->cameras.Has(id)) {
        return impl_->alloc->cameras.Get(id);
    } else {
        return nullptr;
    }
}

void* Renderer::GetMeshPointer(MeshId id) {
    if (impl_->alloc->renderables.Has(id)) {
        return &impl_->alloc->renderables.Get(id);
    } else {
        return nullptr;
    }
}

// ----------------------------------------------------------------------------
struct RendererView::Impl {
    Renderer& renderer;
    const Renderer::ViewId viewId;
    std::unique_ptr<RendererCamera> camera;
    std::unique_ptr<RendererScene> scene;

    Impl(Renderer& r, Renderer::ViewId vid) : renderer(r), viewId(vid) {}
};

RendererView::RendererView(Renderer& renderer, Renderer::ViewId id)
: impl_(new RendererView::Impl(renderer, id))
{
    if (auto view = (filament::View*)impl_->renderer.GetViewPointer(impl_->viewId)) {
        impl_->camera = std::make_unique<RendererCamera>(renderer);
        auto camera = (filament::Camera*)impl_->renderer.GetCameraPointer(impl_->camera->GetId());
        view->setCamera(camera);

        impl_->scene = std::make_unique<RendererScene>(renderer);
        auto scene = (filament::Scene*)impl_->renderer.GetScenePointer(impl_->scene->GetId());
        view->setScene(scene);

        // Set defaults
        view->setClearColor(LinearColorA{ 0.0f, 0.0f, 0.0f, 1.0f });
        GetCamera().SetProjection(0.01, 50, 90.0);
    }
}

RendererView::~RendererView() {
    if (auto view = (filament::View*)impl_->renderer.GetViewPointer(impl_->viewId)) {
        view->setCamera(nullptr);
        view->setScene(nullptr);
    }
    impl_->renderer.DestroyView(impl_->viewId);
}

RendererScene& RendererView::GetScene() {
    return *impl_->scene;
}

RendererCamera& RendererView::GetCamera() {
    return *impl_->camera;
}

void RendererView::SetClearColor(const Color &c) {
    if (auto view = (filament::View*)impl_->renderer.GetViewPointer(impl_->viewId)) {
        view->setClearColor(LinearColorA{ c.GetRed(), c.GetGreen(), c.GetBlue(), c.GetAlpha() });
    }
}

void RendererView::SetViewport(const Rect &r) {
    if (auto view = (filament::View*)impl_->renderer.GetViewPointer(impl_->viewId)) {
        view->setViewport(Viewport(r.x, r.y, r.width, r.height));
        GetCamera().ResizeProjection(float(r.width) / float(r.height));
    }
}

void RendererView::Draw() {
    impl_->renderer.Render(impl_->viewId);
}

// ----------------------------------------------------------------------------
struct RendererCamera::Impl {
    Renderer& renderer;
    Renderer::CameraId cameraId = BOGUS_ID;
    float aspectRatio = -0.0;  // invalid
    float near = 0.01f;
    float far = 50.0f;
    float verticalFoV = 90.0f;

    Impl(Renderer& r) : renderer(r) {
    }
};

RendererCamera::RendererCamera(Renderer& renderer)
: impl_(new RendererCamera::Impl(renderer)) {
    impl_->cameraId = renderer.CreateCamera();
}

RendererCamera::~RendererCamera() {
    impl_->renderer.DestroyCamera(impl_->cameraId);
}

Renderer::CameraId RendererCamera::GetId() const {
    return impl_->cameraId;
}

void RendererCamera::ResizeProjection(float aspectRatio) {
    if (auto camera = (filament::Camera*)impl_->renderer.GetCameraPointer(impl_->cameraId)) {
        impl_->aspectRatio = aspectRatio;
        camera->setProjection(impl_->verticalFoV, aspectRatio, impl_->near, impl_->far,
                              Camera::Fov::VERTICAL);
    }
}

void RendererCamera::SetProjection(float near, float far, float verticalFoV) {
    if (auto camera = (filament::Camera*)impl_->renderer.GetCameraPointer(impl_->cameraId)) {
        impl_->near = near;
        impl_->far = far;
        impl_->verticalFoV = verticalFoV;
        if (impl_->aspectRatio > 0.0) {
            camera->setProjection(verticalFoV, impl_->aspectRatio, near, far, Camera::Fov::VERTICAL);
        }
    }
}

void RendererCamera::LookAt(float eyeX, float eyeY, float eyeZ,
                            float centerX, float centerY, float centerZ,
                            float upX, float upY, float upZ) {
    if (auto camera = (filament::Camera*)impl_->renderer.GetCameraPointer(impl_->cameraId)) {
        camera->lookAt(math::float3{ eyeX, eyeY, eyeZ },
                       math::float3{ centerX, centerY, centerZ},
                       math::float3{ upX, upY, upZ });
    }
}
// ----------------------------------------------------------------------------
struct RendererScene::Impl {
    Renderer& renderer;
    Renderer::SceneId sceneId = BOGUS_ID;

    Impl(Renderer& r) : renderer(r) {
    }
};

RendererScene::RendererScene(Renderer& renderer)
: impl_(new RendererScene::Impl(renderer)) {
    impl_->sceneId = renderer.CreateScene();
}

RendererScene::~RendererScene() {
    impl_->renderer.DestroyScene(impl_->sceneId);
}

Renderer::SceneId RendererScene::GetId() const {
    return impl_->sceneId;
}

void RendererScene::AddIBL(Renderer::IBLId iblId) {
    std::cout << "TODO: implement RendererScene::AddIBL()" << std::endl;
}

void RendererScene::AddLight(Renderer::LightId lightId) {
    std::cout << "TODO: implement RendererScene::AddLight()" << std::endl;
}

void RendererScene::RemoveLight(Renderer::LightId lightId) {
    std::cout << "TODO: implement RendererScene::RemoveLight()" << std::endl;
}

void RendererScene::AddMesh(Renderer::MeshId meshId/*, const Transform& transform*/) {
    auto scene = (filament::Scene*)impl_->renderer.GetScenePointer(impl_->sceneId);
    if (!scene) {
        return;
    }
    if (auto mesh = (utils::Entity*)impl_->renderer.GetMeshPointer(meshId)) {
        scene->addEntity(*mesh);
    }
}

void RendererScene::RemoveMesh(Renderer::MeshId meshId) {
    std::cout << "TODO: implement RendererScene::RemoveMesh()" << std::endl;
}

} // gui
} // open3d
