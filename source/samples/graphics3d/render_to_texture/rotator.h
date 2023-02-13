// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include <dviglo/scene/logic_component.h>

// All Urho3D classes reside in namespace dviglo
using namespace dviglo;

/// Custom logic component for rotating a scene node.
class Rotator : public LogicComponent
{
    DV_OBJECT(Rotator, LogicComponent);

public:
    /// Construct.
    explicit Rotator(Context* context);

    /// Set rotation speed about the Euler axes. Will be scaled with scene update time step.
    void SetRotationSpeed(const Vector3& speed);
    /// Handle scene update. Called by LogicComponent base class.
    void Update(float timeStep) override;

    /// Return rotation speed.
    const Vector3& GetRotationSpeed() const { return rotationSpeed_; }

private:
    /// Rotation speed.
    Vector3 rotationSpeed_;
};
