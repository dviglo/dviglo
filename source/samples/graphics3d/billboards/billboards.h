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

/// Billboard example.
/// This sample demonstrates:
///     - Populating a 3D scene with billboard sets and several shadow casting spotlights
///     - Parenting scene nodes to allow more intuitive creation of groups of objects
///     - Examining rendering performance with a somewhat large object and light count
class Billboards : public Sample
{
    DV_OBJECT(Billboards, Sample);

public:
    /// Construct.
    explicit Billboards();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    /// Subscribe to application-wide logic update and post-render update events.
    void SubscribeToEvents();
    /// Read input and moves the camera.
    void MoveCamera(float timeStep);
    /// Animate the scene.
    void AnimateScene(float timeStep);
    /// Handle the logic update event.
    void handle_update(StringHash eventType, VariantMap& eventData);
    /// Handle the post-render update event.
    void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);

    /// Flag for drawing debug geometry.
    bool drawDebug_;
};
