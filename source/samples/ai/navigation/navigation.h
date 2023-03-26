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

/// Navigation example.
/// This sample demonstrates:
///     - Generating a navigation mesh into the scene
///     - Performing path queries to the navigation mesh
///     - Rebuilding the navigation mesh partially when adding or removing objects
///     - Visualizing custom debug geometry
///     - Raycasting drawable components
///     - Making a node follow the Detour path
class Navigation : public Sample
{
    DV_OBJECT(Navigation, Sample);

public:
    /// Construct.
    explicit Navigation();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void create_scene();
    /// Construct user interface elements.
    void create_ui();
    /// Set up a viewport for displaying the scene.
    void setup_viewport();
    /// Subscribe to application-wide logic update and post-render update events.
    void subscribe_to_events();
    /// Read input and moves the camera.
    void move_camera(float timeStep);
    /// Set path start or end point.
    void SetPathPoint();
    /// Add or remove object.
    void AddOrRemoveObject();
    /// Create a mushroom object at position.
    Node* create_mushroom(const Vector3& pos);
    /// Utility function to raycast to the cursor position. Return true if hit
    bool Raycast(float maxDistance, Vector3& hitPos, Drawable*& hitDrawable);
    /// Make Jack follow the Detour path.
    void FollowPath(float timeStep);
    /// Toggle navigation mesh streaming.
    void toggle_streaming(bool enabled);
    /// Update navigation mesh streaming.
    void update_streaming();
    /// Save navigation data for streaming.
    void save_navigation_data();
    /// Handle the logic update event.
    void handle_update(StringHash eventType, VariantMap& eventData);
    /// Handle the post-render update event.
    void handle_post_render_update(StringHash eventType, VariantMap& eventData);

    /// Last calculated path.
    Vector<Vector3> currentPath_;
    /// Path end position.
    Vector3 endPos_;
    /// Jack scene node.
    SharedPtr<Node> jackNode_;
    /// Flag for drawing debug geometry.
    bool draw_debug_;
    /// Flag for using navigation mesh streaming.
    bool useStreaming_;
    /// Streaming distance.
    int streamingDistance_;
    /// Tile data.
    HashMap<IntVector2, Vector<byte>> tileData_;
    /// Added tiles.
    HashSet<IntVector2> addedTiles_;
};
