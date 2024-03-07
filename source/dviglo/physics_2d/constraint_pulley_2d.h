// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "constraint_2d.h"

namespace dviglo
{

/// 2D pulley constraint component.
class DV_API ConstraintPulley2D : public Constraint2D
{
    DV_OBJECT(ConstraintPulley2D);

public:
    /// Construct.
    explicit ConstraintPulley2D();
    /// Destruct.
    ~ConstraintPulley2D() override;
    /// Register object factory.
    static void register_object();

    /// Set other body ground anchor point.
    void SetOwnerBodyGroundAnchor(const Vector2& groundAnchor);
    /// Set other body ground anchor point.
    void SetOtherBodyGroundAnchor(const Vector2& groundAnchor);
    /// Set owner body anchor point.
    void SetOwnerBodyAnchor(const Vector2& anchor);
    /// Set other body anchor point.
    void SetOtherBodyAnchor(const Vector2& anchor);
    /// Set ratio.
    void SetRatio(float ratio);

    /// Return owner body ground anchor.
    const Vector2& GetOwnerBodyGroundAnchor() const { return ownerBodyGroundAnchor_; }

    /// return other body ground anchor.
    const Vector2& GetOtherBodyGroundAnchor() const { return otherBodyGroundAnchor_; }

    /// Return owner body anchor.
    const Vector2& GetOwnerBodyAnchor() const { return ownerBodyAnchor_; }

    /// Return other body anchor.
    const Vector2& GetOtherBodyAnchor() const { return otherBodyAnchor_; }

    /// Return ratio.
    float GetRatio() const { return jointDef_.ratio; }


private:
    /// Return Joint def.
    b2JointDef* GetJointDef() override;

    /// Box2D joint def.
    b2PulleyJointDef jointDef_;
    /// Owner body ground anchor.
    Vector2 ownerBodyGroundAnchor_;
    /// Other body ground anchor.
    Vector2 otherBodyGroundAnchor_;
    /// Owner body anchor.
    Vector2 ownerBodyAnchor_;
    /// Other body anchor.
    Vector2 otherBodyAnchor_;
};

}
