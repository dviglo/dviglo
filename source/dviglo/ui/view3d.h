// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../ui/window.h"

namespace dviglo
{

class Camera;
class Node;
class Scene;
class Texture2D;
class Viewport;

/// %UI element which renders a 3D scene.
class DV_API View3D : public Window
{
    DV_OBJECT(View3D, Window);

public:
    /// Construct.
    explicit View3D(Context* context);
    /// Destruct.
    ~View3D() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// React to resize.
    void OnResize(const IntVector2& newSize, const IntVector2& delta) override;

    /// Define the scene and camera to use in rendering. When ownScene is true the View3D will take ownership of them with shared pointers.
    void SetView(Scene* scene, Camera* camera, bool ownScene = true);
    /// Set render texture pixel format. Default is RGB.
    void SetFormat(unsigned format);
    /// Set render target auto update mode. Default is true.
    void SetAutoUpdate(bool enable);
    /// Queue manual update on the render texture.
    void QueueUpdate();

    /// Return render texture pixel format.
    unsigned GetFormat() const { return rttFormat_; }

    /// Return whether render target updates automatically.
    bool GetAutoUpdate() const { return autoUpdate_; }

    /// Return scene.
    Scene* GetScene() const;
    /// Return camera scene node.
    Node* GetCameraNode() const;
    /// Return render texture.
    Texture2D* GetRenderTexture() const;
    /// Return depth stencil texture.
    Texture2D* GetDepthTexture() const;
    /// Return viewport.
    Viewport* GetViewport() const;

private:
    /// Reset scene.
    void ResetScene();
    /// Handle render surface update event. Queue the texture for update in case the View3D is visible and automatic update is enabled.
    void HandleRenderSurfaceUpdate(StringHash eventType, VariantMap& eventData);

    /// Renderable texture.
    SharedPtr<Texture2D> renderTexture_;
    /// Depth stencil texture.
    SharedPtr<Texture2D> depthTexture_;
    /// Viewport.
    SharedPtr<Viewport> viewport_;
    /// Scene.
    SharedPtr<Scene> scene_;
    /// Camera scene node.
    SharedPtr<Node> cameraNode_;
    /// Own scene.
    bool ownScene_;
    /// Render texture format.
    unsigned rttFormat_;
    /// Render texture auto update mode.
    bool autoUpdate_;
};

}
