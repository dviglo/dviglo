// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"
#include "../containers/ptr.h"
#include "../math/ray.h"
#include "../math/rect.h"
#include "../math/vector2.h"

namespace dviglo
{

class Camera;
class RenderPath;
class Scene;
class XmlFile;
class View;

/// %Viewport definition either for a render surface or the backbuffer.
class DV_API Viewport : public Object
{
    DV_OBJECT(Viewport);

public:
    bool draw_debug = true;

    /// View rectangle. A zero rectangle (0 0 0 0) means to use the rendertarget's full dimensions.
    /// In this case you could fetch the actual view rectangle from View object, though it will be valid only after the first frame
    IntRect rect;

    /// Construct with defaults.
    explicit Viewport();
    /// Construct with a full rectangle.
    Viewport(Scene* scene, Camera* camera, RenderPath* renderPath = nullptr);
    /// Construct with a specified rectangle.
    Viewport(Scene* scene, Camera* camera, const IntRect& rect, RenderPath* renderPath = nullptr);
    /// Destruct.
    ~Viewport() override;

    /// Set scene.
    void SetScene(Scene* scene);
    /// Set viewport camera.
    void SetCamera(Camera* camera);

    /// Set rendering path.
    void SetRenderPath(RenderPath* renderPath);
    /// Set rendering path from an XML file.
    bool SetRenderPath(XmlFile* file);

    /// Set separate camera to use for culling. Sharing a culling camera between several viewports allows to prepare the view only once, saving in CPU use. The culling camera's frustum should cover all the viewport cameras' frusta or else objects may be missing from the rendered view.
    void SetCullCamera(Camera* camera);

    /// Return scene.
    Scene* GetScene() const;
    /// Return viewport camera.
    Camera* GetCamera() const;
    /// Return the internal rendering structure. May be null if the viewport has not been rendered yet.
    View* GetView() const;

    /// Return rendering path.
    RenderPath* GetRenderPath() const;

    /// Return the culling camera. If null, the viewport camera will be used for culling (normal case).
    Camera* GetCullCamera() const;

    /// Return ray corresponding to normalized screen coordinates.
    Ray GetScreenRay(int x, int y) const;
    /// Convert a world space point to normalized screen coordinates.
    IntVector2 world_to_screen_point(const Vector3& worldPos) const;
    /// Convert screen coordinates and depth to a world space point.
    Vector3 screen_to_world_point(int x, int y, float depth) const;

    /// Allocate the view structure. Called by Renderer.
    void allocate_view();

private:
    /// Scene pointer.
    WeakPtr<Scene> scene_;
    /// Camera pointer.
    WeakPtr<Camera> camera_;
    /// Culling camera pointer.
    WeakPtr<Camera> cullCamera_;

    /// Rendering path.
    SharedPtr<RenderPath> renderPath_;
    /// Internal rendering structure.
    std::unique_ptr<View> view_;
};

}
