// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "constraint_2d.h"

namespace dviglo
{

/// 2D mouse constraint component.
class DV_API ConstraintMouse2D : public Constraint2D
{
    DV_OBJECT(ConstraintMouse2D);

public:
    /// Construct.
    explicit ConstraintMouse2D();

    /// Destruct.
    ~ConstraintMouse2D() override;

    /// Register object factory.
    static void register_object();

    /// Set target.
    void SetTarget(const Vector2& target);

    /// Return target.
    const Vector2& GetTarget() const { return target_; }

    /// Set max force.
    void SetMaxForce(float maxForce);

    /// Return max force.
    float GetMaxForce() const { return jointDef_.maxForce; }

    /// Set linear stiffness in N/m.
    void SetStiffness(float stiffness);

    /// Return linear stiffness in N/m.
    float GetStiffness() const { return jointDef_.stiffness; }

    /// Set linear damping in N*s/m.
    void SetDamping(float damping);

    /// Return linear damping in N*s/m.
    float GetDamping() const { return jointDef_.damping; }

    /// Calc and set stiffness and damping. Must be used after set owner and other bodies.
    bool SetLinearStiffness(float frequencyHertz, float dampingRatio);

private:
    /// Return joint def.
    b2JointDef* GetJointDef() override;

    /// Box2D joint def.
    b2MouseJointDef jointDef_;

    /// Target.
    Vector2 target_;
};

}
