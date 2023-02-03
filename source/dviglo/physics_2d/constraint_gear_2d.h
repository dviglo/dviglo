// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../physics_2d/constraint_2d.h"

namespace Urho3D
{

/// 2D gear constraint component.
class URHO3D_API ConstraintGear2D : public Constraint2D
{
    URHO3D_OBJECT(ConstraintGear2D, Constraint2D);

public:
    /// Construct.
    explicit ConstraintGear2D(Context* context);
    /// Destruct.
    ~ConstraintGear2D() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Set owner constraint.
    void SetOwnerConstraint(Constraint2D* constraint);
    /// Set other constraint.
    void SetOtherConstraint(Constraint2D* constraint);
    /// Set ratio.
    void SetRatio(float ratio);

    /// Return owner constraint.
    Constraint2D* GetOwnerConstraint() const { return ownerConstraint_; }

    /// Return other constraint.
    Constraint2D* GetOtherConstraint() const { return otherConstraint_; }

    /// Return ratio.
    float GetRatio() const { return jointDef_.ratio; }

private:
    /// Return joint def.
    b2JointDef* GetJointDef() override;

    /// Box2D joint def.
    b2GearJointDef jointDef_;
    /// Owner body constraint.
    WeakPtr<Constraint2D> ownerConstraint_;
    /// Other body constraint.
    WeakPtr<Constraint2D> otherConstraint_;
};

}
