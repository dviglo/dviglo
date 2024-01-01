// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "constraint_2d.h"

namespace dviglo
{

/// 2D friction constraint component.
class DV_API ConstraintFriction2D : public Constraint2D
{
    DV_OBJECT(ConstraintFriction2D);

public:
    /// Construct.
    explicit ConstraintFriction2D();
    /// Destruct.
    ~ConstraintFriction2D() override;
    /// Register object factory.
    static void register_object();

    /// Set anchor.
    void SetAnchor(const Vector2& anchor);
    /// Set max force.
    void SetMaxForce(float maxForce);
    /// Set max torque.
    void SetMaxTorque(float maxTorque);

    /// Return anchor.
    const Vector2& GetAnchor() const { return anchor_; }

    /// Set max force.
    float GetMaxForce() const { return jointDef_.maxForce; }

    /// Set max torque.
    float GetMaxTorque() const { return jointDef_.maxTorque; }

private:
    /// Return joint def.
    b2JointDef* GetJointDef() override;

    /// Box2D joint def.
    b2FrictionJointDef jointDef_;
    /// Anchor.
    Vector2 anchor_;
};

}
