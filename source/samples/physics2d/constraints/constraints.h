// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../Sample.h"

namespace dviglo
{
    class Node;
    class Scene;
    class ConstraintDistance2D;
    class ConstraintFriction2D;
    class ConstraintGear2D;
    class ConstraintMotor2D;
    class ConstraintMouse2D;
    class ConstraintPrismatic2D;
    class ConstraintPulley2D;
    class ConstraintRevolute2D;
    class ConstraintRope2D;
    class ConstraintWeld2D;
    class ConstraintWheel2D;
}

/// Physics2D constraints sample.
/// This sample is designed to help understanding and chosing the right constraint.
/// This sample demonstrates:
///     - Creating physics constraints
///     - Creating Edge and Polygon Shapes from vertices
///     - Displaying physics debug geometry and constraints' joints
///     - Using SetOrderInLayer to alter the way sprites are drawn in relation to each other
///     - Using Text3D to display some text affected by zoom
///     - Setting the background color for the scene
class Urho2DConstraints : public Sample
{
    DV_OBJECT(Urho2DConstraints, Sample);

public:
    /// Construct.
    explicit Urho2DConstraints();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void create_scene();
    /// Construct an instruction text to the UI.
    void create_instructions();
    /// Create Tex3D flag.
    void CreateFlag(const String& text, float x, float y);
    /// Read input and moves the camera.
    void move_camera(float timeStep);
    /// Subscribe to application-wide logic update events.
    void subscribe_to_events();
    /// Handle the logic update event.
    void handle_update(StringHash eventType, VariantMap& eventData);
    /// Handle the post render update event during which we request debug geometry.
    void handle_post_render_update(StringHash eventType, VariantMap& eventData);
    /// Handle the mouse click event.
    void HandleMouseButtonDown(StringHash eventType, VariantMap& eventData);
    /// Handle the mouse button up event.
    void HandleMouseButtonUp(StringHash eventType, VariantMap& eventData);
    /// Handle the mouse move event.
    void HandleMouseMove(StringHash eventType, VariantMap& eventData);

    /// Get mouse position in 2D world coordinates.
    Vector2 GetMousePositionXY();
    /// Flag for drawing debug geometry.
    bool drawDebug_{};
    /// Camera object.
    Camera* camera_{};
};
