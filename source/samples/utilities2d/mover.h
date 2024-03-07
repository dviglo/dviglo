// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include <dviglo/scene/logic_component.h>

// All Urho3D classes reside in namespace dviglo
using namespace dviglo;

/// Mover logic component
///    - Handles entity (enemy, platform...) translation along a path (set of Vector2 points)
///    - Supports looping paths and animation flip
///    - Default speed is 0.8 if 'Speed' property is not set in the tmx file objects
class Mover : public LogicComponent
{
    DV_OBJECT(Mover);

public:
    /// Construct.
    explicit Mover();

    /// Register object factory and attributes.
    static void register_object();

    /// Handle scene update. Called by LogicComponent base class.
    void Update(float timeStep) override;
    /// Return path attribute.
    Vector<byte> GetPathAttr() const;
    /// Set path attribute.
    void SetPathAttr(const Vector<byte>& value);

    /// Path.
    Vector<Vector2> path_;
    /// Movement speed.
    float speed_;
    /// ID of the current path point.
    int currentPathID_;
    /// Timer for particle emitter duration.
    float emitTime_;
    /// Timer used for handling "attack" animation.
    float fightTimer_;
    /// Flip animation based on direction, or player position when fighting.
    float flip_;
};
