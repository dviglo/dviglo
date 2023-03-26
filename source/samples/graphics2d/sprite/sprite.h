// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../Sample.h"

namespace dviglo
{
    class Node;
    class Scene;
}

/// Urho2D sprite example.
/// This sample demonstrates:
///     - Creating a 2D scene with sprite
///     - Displaying the scene using the Renderer subsystem
///     - Handling keyboard to move and zoom 2D camera
class Urho2DSprite : public Sample
{
    DV_OBJECT(Urho2DSprite, Sample);

public:
    /// Construct.
    explicit Urho2DSprite();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void create_scene();
    /// Construct an instruction text to the UI.
    void create_instructions();
    /// Set up a viewport for displaying the scene.
    void setup_viewport();
    /// Read input and moves the camera.
    void move_camera(float timeStep);
    /// Subscribe to application-wide logic update events.
    void subscribe_to_events();
    /// Handle the logic update event.
    void handle_update(StringHash eventType, VariantMap& eventData);

    /// Sprite nodes.
    Vector<SharedPtr<Node>> spriteNodes_;
};
