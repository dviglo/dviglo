// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../Sample.h"

namespace dviglo
{

class Drawable;
class Node;
class Scene;

}

/// Scene & UI load example.
/// This sample demonstrates:
///      - Loading a scene from a file and showing it
///      - Loading a UI layout from a file and showing it
///      - Subscribing to the UI layout's events
class SceneAndUILoad : public Sample
{
    DV_OBJECT(SceneAndUILoad, Sample);

public:
    /// Construct.
    explicit SceneAndUILoad();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void create_scene();
    /// Construct user interface elements.
    void CreateUI();
    /// Set up a viewport for displaying the scene.
    void setup_viewport();
    /// Subscribe to application-wide logic update event.
    void subscribe_to_events();
    /// Reads input and moves the camera.
    void MoveCamera(float timeStep);
    /// Handle the logic update event.
    void handle_update(StringHash eventType, VariantMap& eventData);
    /// Handle toggle button 1 being pressed.
    void ToggleLight1(StringHash eventType, VariantMap& eventData);
    /// Handle toggle button 2 being pressed.
    void ToggleLight2(StringHash eventType, VariantMap& eventData);
};
