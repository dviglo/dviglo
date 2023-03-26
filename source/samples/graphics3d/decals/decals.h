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

/// Decals example.
/// This sample demonstrates:
///     - Performing a raycast to the octree and adding a decal to the hit location
///     - Defining a Cursor UI element which stays inside the window and can be shown/hidden
///     - Marking suitable (large) objects as occluders for occlusion culling
///     - Displaying renderer debug geometry to see the effect of occlusion
class Decals : public Sample
{
    DV_OBJECT(Decals, Sample);

public:
    /// Construct.
    explicit Decals();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void create_scene();
    /// Construct user interface elements.
    void CreateUI();
    /// Set up a viewport for displaying the scene.
    void setup_viewport();
    /// Subscribe to application-wide logic update and post-render update events.
    void subscribe_to_events();
    /// Reads input and moves the camera.
    void MoveCamera(float timeStep);
    /// Paint a decal using a ray cast from the mouse cursor.
    void PaintDecal();
    /// Utility function to raycast to the cursor position. Return true if hit
    bool Raycast(float maxDistance, Vector3& hitPos, Drawable*& hitDrawable);
    /// Handle the logic update event.
    void handle_update(StringHash eventType, VariantMap& eventData);
    /// Handle the post-render update event.
    void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);

    /// Flag for drawing debug geometry.
    bool drawDebug_;
};
