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

/// Urho2D and Physics2D sample.
/// This sample demonstrates:
///     - Creating both static and moving 2D physics objects to a scene
///     - Displaying physics debug geometry
class Urho2DPhysics : public Sample
{
    DV_OBJECT(Urho2DPhysics, Sample);

public:
    /// Construct.
    explicit Urho2DPhysics();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void create_instructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    /// Read input and moves the camera.
    void MoveCamera(float timeStep);
    /// Subscribe to application-wide logic update events.
    void SubscribeToEvents();
    /// Handle the logic update event.
    void handle_update(StringHash eventType, VariantMap& eventData);
};
