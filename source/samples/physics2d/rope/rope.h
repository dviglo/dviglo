// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../Sample.h"

namespace dviglo
{
    class Node;
    class Scene;
    class ConstraintRope2D;
}

/// Physics2D rope sample.
/// This sample demonstrates:
///     - Create revolute constraint
///     - Create roop constraint
///     - Displaying physics debug geometry
class Urho2DPhysicsRope : public Sample
{
    DV_OBJECT(Urho2DPhysicsRope, Sample);

public:
    /// Construct.
    explicit Urho2DPhysicsRope();

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
