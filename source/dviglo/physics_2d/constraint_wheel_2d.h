// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "constraint_2d.h"

namespace dviglo
{

/// 2D wheel constraint component.
class DV_API ConstraintWheel2D : public Constraint2D
{
    DV_OBJECT(ConstraintWheel2D);

public:
    /// Construct.
    explicit ConstraintWheel2D();

    /// Destruct.
    ~ConstraintWheel2D() override;

    /// Register object factory.
    static void register_object();

    /// Set anchor.
    void SetAnchor(const Vector2& anchor);

    /// Return anchor.
    const Vector2& GetAnchor() const { return anchor_; }

    /// Set axis.
    void SetAxis(const Vector2& axis);

    /// Return axis.
    const Vector2& GetAxis() const { return axis_; }

    /// Set enable motor.
    void SetEnableMotor(bool enableMotor);

    /// Return enable motor.
    bool GetEnableMotor() const { return jointDef_.enableMotor; }

    /// Set max motor torque.
    void SetMaxMotorTorque(float maxMotorTorque);

    /// Return maxMotor torque.
    float GetMaxMotorTorque() const { return jointDef_.maxMotorTorque; }

    /// Set motor speed.
    void SetMotorSpeed(float motorSpeed);

    /// Return motor speed.
    float GetMotorSpeed() const { return jointDef_.motorSpeed; }

    /// Set linear stiffness in N/m.
    void SetStiffness(float stiffness);

    /// Return linear stiffness in N/m.
    float GetStiffness() const { return jointDef_.stiffness; }

    /// Set linear damping in N*s/m.
    void SetDamping(float damping);

    /// Return linear damping in N*s/m.
    float GetDamping() const { return jointDef_.damping; }

    /// Set enable limit.
    void SetEnableLimit(bool enableLimit);

    /// Return enable limit.
    bool GetEnableLimit()  const { return jointDef_.enableLimit; }

    /// Set lower translation.
    void SetLowerTranslation(float lowerTranslation);

    /// Return lower translation.
    float GetLowerTranslation() const { return jointDef_.lowerTranslation; }

    /// Set upper translation.
    void SetUpperTranslation(float upperTranslation);

    /// Return upper translation.
    float GetUpperTranslation() const { return jointDef_.upperTranslation; }

    /// Calc and set stiffness and damping. Must be used after set owner and other bodies.
    bool SetLinearStiffness(float frequencyHertz, float dampingRatio);

private:
    /// Return joint def.
    b2JointDef* GetJointDef() override;

    /// Box2D joint def.
    b2WheelJointDef jointDef_;

    /// Anchor.
    Vector2 anchor_;

    /// Axis.
    Vector2 axis_;
};

}
