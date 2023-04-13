// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../sample.h"

/// Moving sprites example.
/// This sample demonstrates:
///     - Adding Sprite elements to the UI
///     - Storing custom data (sprite velocity) inside UI elements
///     - Handling frame update events in which the sprites are moved
class Sprites : public Sample
{
    // Enable type information.
    DV_OBJECT(Sprites);

public:
    /// Construct.
    explicit Sprites();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the sprites.
    void CreateSprites();
    /// Move the sprites using the delta time step given.
    void MoveSprites(float timeStep);
    /// Subscribe to application-wide logic update events.
    void subscribe_to_events();
    /// Handle the logic update event.
    void handle_update(StringHash eventType, VariantMap& eventData);

    /// Vector to store the sprites for iterating through them.
    Vector<SharedPtr<Sprite>> sprites_;
};
