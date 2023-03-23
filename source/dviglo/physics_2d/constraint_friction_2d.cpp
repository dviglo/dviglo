// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "constraint_friction_2d.h"
#include "physics_utils_2d.h"
#include "rigid_body_2d.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* PHYSICS2D_CATEGORY;

ConstraintFriction2D::ConstraintFriction2D() :
    anchor_(Vector2::ZERO)
{

}

ConstraintFriction2D::~ConstraintFriction2D() = default;

void ConstraintFriction2D::register_object()
{
    DV_CONTEXT.RegisterFactory<ConstraintFriction2D>(PHYSICS2D_CATEGORY);

    DV_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, true, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Anchor", GetAnchor, SetAnchor, Vector2::ZERO, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Max Force", GetMaxForce, SetMaxForce, 0.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Max Torque", GetMaxTorque, SetMaxTorque, 0.0f, AM_DEFAULT);
    DV_COPY_BASE_ATTRIBUTES(Constraint2D);
}

void ConstraintFriction2D::SetAnchor(const Vector2& anchor)
{
    if (anchor == anchor_)
        return;

    anchor_ = anchor;

    RecreateJoint();
    MarkNetworkUpdate();
}

void ConstraintFriction2D::SetMaxForce(float maxForce)
{
    if (maxForce == jointDef_.maxForce)
        return;

    jointDef_.maxForce = maxForce;

    if (joint_)
        static_cast<b2FrictionJoint*>(joint_)->SetMaxForce(maxForce);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}


void ConstraintFriction2D::SetMaxTorque(float maxTorque)
{
    if (maxTorque == jointDef_.maxTorque)
        return;

    jointDef_.maxTorque = maxTorque;

    if (joint_)
        static_cast<b2FrictionJoint*>(joint_)->SetMaxTorque(maxTorque);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

b2JointDef* ConstraintFriction2D::GetJointDef()
{
    if (!ownerBody_ || !otherBody_)
        return nullptr;

    b2Body* bodyA = ownerBody_->GetBody();
    b2Body* bodyB = otherBody_->GetBody();
    if (!bodyA || !bodyB)
        return nullptr;

    jointDef_.Initialize(bodyA, bodyB, ToB2Vec2(anchor_));

    return &jointDef_;
}

}
