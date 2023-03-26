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

/// CrowdNavigation example.
/// This sample demonstrates:
///     - Generating a dynamic navigation mesh into the scene
///     - Performing path queries to the navigation mesh
///     - Adding and removing obstacles/agents at runtime
///     - Raycasting drawable components
///     - Crowd movement management
///     - Accessing crowd agents with the crowd manager
///     - Using off-mesh connections to make boxes climbable
///     - Using agents to simulate moving obstacles
class CrowdNavigation : public Sample
{
    DV_OBJECT(CrowdNavigation, Sample);

public:
    /// Construct.
    explicit CrowdNavigation();

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
    /// Set crowd agents target or spawn another jack.
    void SetPathPoint(bool spawning);
    /// Add new obstacle or remove existing obstacle/agent.
    void AddOrRemoveObject();
    /// Create a "Jack" object at position.
    void SpawnJack(const Vector3& pos, Node* jackGroup);
    /// Create a mushroom object at position.
    void create_mushroom(const Vector3& pos);
    /// Create an off-mesh connection for each box to make it climbable.
    void CreateBoxOffMeshConnections(DynamicNavigationMesh* navMesh, Node* boxGroup);
    /// Create some movable barrels as crowd agents.
    void CreateMovingBarrels(DynamicNavigationMesh* navMesh);
    /// Utility function to raycast to the cursor position. Return true if hit.
    bool Raycast(float maxDistance, Vector3& hitPos, Drawable*& hitDrawable);
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
    /// Handle problems with crowd agent placement.
    void HandleCrowdAgentFailure(StringHash eventType, VariantMap& eventData);
    /// Handle crowd agent reposition.
    void HandleCrowdAgentReposition(StringHash eventType, VariantMap& eventData);
    /// Handle crowd agent formation.
    void HandleCrowdAgentFormation(StringHash eventType, VariantMap& eventData);

    /// Flag for using navigation mesh streaming.
    bool useStreaming_{};
    /// Streaming distance.
    int streamingDistance_{2};
    /// Tile data.
    HashMap<IntVector2, Vector<byte>> tileData_;
    /// Added tiles.
    HashSet<IntVector2> addedTiles_;
    /// Flag for drawing debug geometry.
    bool drawDebug_{};
    /// Instruction text UI-element.
    Text* instructionText_{};
};
