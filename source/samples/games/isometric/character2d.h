// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include <dviglo/scene/logic_component.h>

// All Urho3D classes reside in namespace dviglo
using namespace dviglo;

const float MOVE_SPEED_X = 4.0f;
const int LIFES = 3;

/// Character2D component controling Imp behavior.
class Character2D : public LogicComponent
{
    DV_OBJECT(Character2D, LogicComponent);

public:
    /// Construct.
    explicit Character2D();

    /// Register object factory and attributes.
    static void RegisterObject();

    /// Handle update. Called by LogicComponent base class.
    void Update(float timeStep) override;

    /// Handle player state/behavior when wounded.
    void HandleWoundedState(float timeStep);

    /// Handle death of the player.
    void HandleDeath();

    /// Flag when player is wounded.
    bool wounded_;
    /// Flag when player is dead.
    bool killed_;
    /// Timer for particle emitter duration.
    float timer_;
    /// Number of coins in the current level.
    int maxCoins_;
    /// Counter for remaining coins to pick.
    int remainingCoins_;
    /// Counter for remaining lifes.
    int remainingLifes_;
    /// Scaling factor based on tiles' aspect ratio (definitively set at tile map creation).
    float moveSpeedScale_;
    /// Camera's zoom (used to scale movement speed based on camera zoom).
    float zoom_;
};
