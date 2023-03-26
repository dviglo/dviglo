// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../Sample.h"

/// Urho2D tile map example.
/// This sample demonstrates:
///     - Creating a 2D scene with tile map
///     - Displaying the scene using the Renderer subsystem
///     - Handling keyboard to move and zoom 2D camera
///     - Interacting with the tile map
class Urho2DTileMap : public Sample
{
    DV_OBJECT(Urho2DTileMap, Sample);

public:
    /// Construct.
    explicit Urho2DTileMap();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void create_scene();
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
    /// Handle the mouse click event.
    void HandleMouseButtonDown(StringHash eventType, VariantMap& eventData);
    /// Get mouse position in 2D world coordinates.
    Vector2 GetMousePositionXY();
};
