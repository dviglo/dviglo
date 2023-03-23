// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../scene/component.h"

#include <box2d/box2d.h>

namespace dviglo
{

class RigidBody2D;
class PhysicsWorld2D;

/// 2D physics constraint component.
class DV_API Constraint2D : public Component
{
    DV_OBJECT(Constraint2D, Component);

public:
    /// Construct.
    explicit Constraint2D();
    /// Destruct.
    ~Constraint2D() override;
    /// Register object factory.
    static void register_object();

    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    void apply_attributes() override;
    /// Handle enabled/disabled state change.
    void OnSetEnabled() override;
    /// Create joint.
    void CreateJoint();
    /// Release joint.
    void ReleaseJoint();

    /// Set other rigid body.
    void SetOtherBody(RigidBody2D* body);
    /// Set collide connected.
    void SetCollideConnected(bool collideConnected);
    /// Set attached constriant (for gear).
    void SetAttachedConstraint(Constraint2D* constraint);

    /// Return owner body.
    RigidBody2D* GetOwnerBody() const { return ownerBody_; }

    /// Return other body.
    RigidBody2D* GetOtherBody() const { return otherBody_; }

    /// Return collide connected.
    bool GetCollideConnected() const { return collideConnected_; }

    /// Return attached constraint (for gear).
    Constraint2D* GetAttachedConstraint() const { return attachedConstraint_; }

    /// Return Box2D joint.
    b2Joint* GetJoint() const { return joint_; }

protected:
    /// Handle node being assigned.
    void OnNodeSet(Node* node) override;
    /// Handle scene being assigned.
    void OnSceneSet(Scene* scene) override;
    /// Return joint def.
    virtual b2JointDef* GetJointDef() { return nullptr; };
    /// Recreate joint.
    void RecreateJoint();
    /// Initialize joint def.
    void InitializeJointDef(b2JointDef* jointDef);
    /// Mark other body node ID dirty.
    void MarkOtherBodyNodeIDDirty() { otherBodyNodeIDDirty_ = true; }

    /// Physics world.
    WeakPtr<PhysicsWorld2D> physicsWorld_;
    /// Box2D joint.
    b2Joint* joint_{};
    /// Owner body.
    WeakPtr<RigidBody2D> ownerBody_;
    /// Other body.
    WeakPtr<RigidBody2D> otherBody_;
    /// Other body node ID for serialization.
    unsigned otherBodyNodeID_{};
    /// Collide connected flag.
    bool collideConnected_{};
    /// Other body node ID dirty flag.
    bool otherBodyNodeIDDirty_{};
    /// Attached constraint.
    WeakPtr<Constraint2D> attachedConstraint_;
};

}
