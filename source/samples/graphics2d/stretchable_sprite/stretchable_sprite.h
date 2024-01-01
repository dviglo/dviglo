// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "../../sample.h"

namespace dviglo
{
    class Node;
}

/// Urho2D stretchable sprite example.
/// This sample demonstrates:
///     - Creating a 2D scene with both static and stretchable sprites
///     - Difference in scaling static and stretchable sprites
///     - Similarity in otherwise transforming static and stretchable sprites
///     - Displaying the scene using the Renderer subsystem
///     - Handling keyboard to transform nodes
class Urho2DStretchableSprite : public Sample
{
    DV_OBJECT(Urho2DStretchableSprite);

public:
    /// Construct.
    explicit Urho2DStretchableSprite();

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
    /// Handle the logic update event.
    void handle_update(StringHash eventType, VariantMap& eventData);
    /// Handle KeyUp event.
    void OnKeyUp(StringHash eventType, VariantMap& eventData);
    /// Translate sprite nodes.
    void TranslateSprites(float timeStep);
    /// Rotate sprite nodes.
    void RotateSprites(float timeStep);
    /// Scale sprite nodes.
    void ScaleSprites(float timeStep);

    /// Reference (static) sprite node.
    SharedPtr<Node> refSpriteNode_;
    /// Stretchable sprite node.
    SharedPtr<Node> stretchSpriteNode_;
    /// Transform mode tracking index.
    unsigned selectTransform_ = 0;
};
