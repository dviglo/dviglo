// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "../../sample.h"

namespace dviglo
{

class Node;
class Scene;

}

/// Animating 3D scene example.
/// This sample demonstrates:
///     - Creating a 3D scene and using a custom component to animate the objects
///     - Controlling scene ambience with the Zone component
///     - Attaching a light to an object (the camera)
class AnimatingScene : public Sample
{
    DV_OBJECT(AnimatingScene);

public:
    /// Construct.
    explicit AnimatingScene();

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
};
