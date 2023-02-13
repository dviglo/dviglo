// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../physics_2d/constraint_2d.h"

namespace dviglo
{

/// 2D distance constraint component.
class DV_API ConstraintDistance2D : public Constraint2D
{
    DV_OBJECT(ConstraintDistance2D, Constraint2D);

public:
    /// Construct.
    explicit ConstraintDistance2D(Context* context);

    /// Destruct.
    ~ConstraintDistance2D() override;

    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Set owner body anchor.
    void SetOwnerBodyAnchor(const Vector2& anchor);

    /// Return owner body anchor.
    const Vector2& GetOwnerBodyAnchor() const { return ownerBodyAnchor_; }

    /// Set other body anchor.
    void SetOtherBodyAnchor(const Vector2& anchor);

    /// Return other body anchor.
    const Vector2& GetOtherBodyAnchor() const { return otherBodyAnchor_; }

    /// Set linear stiffness in N/m.
    void SetStiffness(float stiffness);

    /// Return linear stiffness in N/m.
    float GetStiffness() const { return jointDef_.stiffness; }

    /// Set linear damping in N*s/m.
    void SetDamping(float damping);

    /// Return linear damping in N*s/m.
    float GetDamping() const { return jointDef_.damping; }

    /// Set length.
    void SetLength(float length);

    /// Return length.
    float GetLength() const { return jointDef_.length; }

    /// Set min length.
    void SetMinLength(float minLength);

    /// Return min length.
    float GetMinLength() const { return jointDef_.minLength; }

    /// Set max length.
    void SetMaxLength(float maxLength);

    /// Return max length.
    float GetMaxLength() const { return jointDef_.maxLength; }

    /// Calc and set stiffness and damping. Must be used after set owner and other bodies.
    bool SetLinearStiffness(float frequencyHertz, float dampingRatio);

private:
    /// Return joint def.
    b2JointDef* GetJointDef() override;

    /// Box2D joint def.
    b2DistanceJointDef jointDef_;

    /// Owner body anchor.
    Vector2 ownerBodyAnchor_;

    /// Other body anchor.
    Vector2 otherBodyAnchor_;
};

}
