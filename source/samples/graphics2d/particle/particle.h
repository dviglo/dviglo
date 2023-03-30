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

// Urho2D particle example.
// This sample demonstrates:
//     - Creating a 2D scene with particle
//     - Displaying the scene using the Renderer subsystem
//     - Handling mouse move to move particle
class Urho2DParticle : public Sample
{
    DV_OBJECT(Urho2DParticle, Sample);

public:
    /// Construct.
    explicit Urho2DParticle();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void create_scene();
    /// Construct an instruction text to the UI.
    void create_instructions();
    /// Set up a viewport for displaying the scene.
    void setup_viewport();
    /// Subscribe to application-wide logic update events.
    void subscribe_to_events();
    /// Handle mouse move event.
    void HandleMouseMove(StringHash eventType, VariantMap& eventData);

    /// Particle scene node.
    SharedPtr<Node> particleNode_;
};
