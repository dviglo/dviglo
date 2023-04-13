// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../sample.h"

namespace dviglo
{

class Node;
class Scene;

}

/// Skeletal animation example.
/// This sample demonstrates:
///     - Populating a 3D scene with skeletally animated AnimatedModel components;
///     - Moving the animated models and advancing their animation using a custom component
///     - Enabling a cascaded shadow map on a directional light, which allows high-quality shadows
///       over a large area (typically used in outdoor scenes for shadows cast by sunlight)
///     - Displaying renderer debug geometry
class SkeletalAnimation : public Sample
{
    DV_OBJECT(SkeletalAnimation);

public:
    /// Construct.
    explicit SkeletalAnimation();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void create_scene();
    /// Construct an instruction text to the UI.
    void create_instructions();
#ifdef DV_GLES3
    /// Create additional lights fo scene
    void CreateLights();
#endif
    /// Set up a viewport for displaying the scene.
    void setup_viewport();
    /// Subscribe to application-wide logic update and post-render update events.
    void subscribe_to_events();
    /// Read input and moves the camera.
    void move_camera(float timeStep);
    /// Handle the logic update event.
    void handle_update(StringHash eventType, VariantMap& eventData);
    /// Handle the post-render update event.
    void handle_post_render_update(StringHash eventType, VariantMap& eventData);

    /// Flag for drawing debug geometry.
    bool draw_debug_;
};
