// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../sample.h"

namespace dviglo
{

class Node;
class Scene;

}

/// Render to texture example
/// This sample demonstrates:
///     - Creating two 3D scenes and rendering the other into a texture
///     - Creating rendertarget texture and material programmatically
class RenderToTexture : public Sample
{
    DV_OBJECT(RenderToTexture, Sample);

public:
    /// Construct.
    explicit RenderToTexture();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void create_scene();
    /// Construct an instruction text to the UI.
    void create_instructions();
    /// Set up a viewport for displaying the scene.
    void setup_viewport();
    /// Subscribe to application-wide logic update events.
    void subscribe_to_events();
    /// Read input and moves the camera.
    void move_camera(float timeStep);
    /// Handle the logic update event.
    void handle_update(StringHash eventType, VariantMap& eventData);

    /// Scene that is rendered to a texture.
    SharedPtr<Scene> rttScene_;
    /// Camera scene node in the render-to-texture scene.
    SharedPtr<Node> rttCameraNode_;
};
