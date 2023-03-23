// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "constraint_distance_2d.h"
#include "physics_utils_2d.h"
#include "rigid_body_2d.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* PHYSICS2D_CATEGORY;

ConstraintDistance2D::ConstraintDistance2D() :
    ownerBodyAnchor_(Vector2::ZERO),
    otherBodyAnchor_(Vector2::ZERO)
{

}

ConstraintDistance2D::~ConstraintDistance2D() = default;

void ConstraintDistance2D::register_object()
{
    DV_CONTEXT.RegisterFactory<ConstraintDistance2D>(PHYSICS2D_CATEGORY);

    DV_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, true, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Owner Body Anchor", GetOwnerBodyAnchor, SetOwnerBodyAnchor, Vector2::ZERO, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Other Body Anchor", GetOtherBodyAnchor, SetOtherBodyAnchor, Vector2::ZERO, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Stiffness", GetStiffness, SetStiffness, 0.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Damping", GetDamping, SetDamping, 0.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Length", GetLength, SetLength, 1.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Min Length", GetMinLength, SetMinLength, 0.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Max Length", GetMaxLength, SetMaxLength, FLT_MAX, AM_DEFAULT);
    DV_COPY_BASE_ATTRIBUTES(Constraint2D);
}

void ConstraintDistance2D::SetOwnerBodyAnchor(const Vector2& anchor)
{
    if (anchor == ownerBodyAnchor_)
        return;

    ownerBodyAnchor_ = anchor;

    RecreateJoint();
    MarkNetworkUpdate();
}

void ConstraintDistance2D::SetOtherBodyAnchor(const Vector2& anchor)
{
    if (anchor == otherBodyAnchor_)
        return;

    otherBodyAnchor_ = anchor;

    RecreateJoint();
    MarkNetworkUpdate();
}

void ConstraintDistance2D::SetStiffness(float stiffness)
{
    if (stiffness == jointDef_.stiffness)
        return;

    jointDef_.stiffness = stiffness;

    if (joint_)
        static_cast<b2DistanceJoint*>(joint_)->SetStiffness(stiffness);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

void ConstraintDistance2D::SetDamping(float damping)
{
    if (damping == jointDef_.damping)
        return;

    jointDef_.damping = damping;

    if (joint_)
        static_cast<b2DistanceJoint*>(joint_)->SetDamping(damping);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

void ConstraintDistance2D::SetLength(float length)
{
    if (length == jointDef_.length)
        return;

    jointDef_.length = length;

    if (joint_)
        static_cast<b2DistanceJoint*>(joint_)->SetLength(length);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

b2JointDef* ConstraintDistance2D::GetJointDef()
{
    if (!ownerBody_ || !otherBody_)
        return nullptr;

    b2Body* bodyA = ownerBody_->GetBody();
    b2Body* bodyB = otherBody_->GetBody();
    if (!bodyA || !bodyB)
        return nullptr;

    jointDef_.Initialize(bodyA, bodyB, ToB2Vec2(ownerBodyAnchor_), ToB2Vec2(otherBodyAnchor_));

    return &jointDef_;
}

bool ConstraintDistance2D::SetLinearStiffness(float frequencyHertz, float dampingRatio)
{
    if (!ownerBody_ || !otherBody_)
        return false;

    b2Body* bodyA = ownerBody_->GetBody();
    b2Body* bodyB = otherBody_->GetBody();
    if (!bodyA || !bodyB)
        return false;

    float stiffness, damping;
    b2LinearStiffness(stiffness, damping, frequencyHertz, dampingRatio, bodyA, bodyB);

    if (joint_)
    {
        static_cast<b2DistanceJoint*>(joint_)->SetDamping(damping);
        static_cast<b2DistanceJoint*>(joint_)->SetStiffness(stiffness);
    }
    else
    {
        RecreateJoint();
    }

    MarkNetworkUpdate();

    return true;
}

void ConstraintDistance2D::SetMinLength(float minLength)
{
    if (minLength == jointDef_.minLength)
        return;

    jointDef_.minLength = minLength;

    if (joint_)
        static_cast<b2DistanceJoint*>(joint_)->SetMinLength(minLength);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

void ConstraintDistance2D::SetMaxLength(float maxLength)
{
    if (maxLength == jointDef_.maxLength)
        return;

    jointDef_.maxLength = maxLength;

    if (joint_)
        static_cast<b2DistanceJoint*>(joint_)->SetMaxLength(maxLength);
    else
        RecreateJoint();

    MarkNetworkUpdate();
}

}
