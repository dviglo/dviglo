// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "../../sample.h"

namespace dviglo
{

class Node;
class Scene;

}

/// Physics example.
/// This sample demonstrates:
///     - Creating both static and moving physics objects to a scene
///     - Displaying physics debug geometry
///     - Using the Skybox component for setting up an unmoving sky
///     - Saving a scene to a file and loading it to restore a previous state
class Physics : public Sample
{
    DV_OBJECT(Physics);

public:
    /// Construct.
    explicit Physics();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void create_scene();
    /// Construct an instruction text to the UI.
    void create_instructions();
    /// Set up a viewport for displaying the scene.
    void setup_viewport();
    /// Subscribe to application-wide logic update and post-render update events.
    void subscribe_to_events();
    /// Read input and moves the camera.
    void move_camera(float timeStep);
    /// Spawn a physics object from the camera position.
    void SpawnObject();
    /// Handle the logic update event.
    void handle_update(StringHash eventType, VariantMap& eventData);
    /// Handle the post-render update event.
    void handle_post_render_update(StringHash eventType, VariantMap& eventData);

    /// Flag for drawing debug geometry.
    bool draw_debug_;
};
