// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

/// \file

#pragma once

#include "../scene/component.h"

#include <box2d/box2d.h>

namespace dviglo
{

class CollisionShape2D;
class Constraint2D;
class PhysicsWorld2D;

/// Rigid body type.
enum BodyType2D
{
    BT_STATIC = b2_staticBody,
    BT_KINEMATIC = b2_kinematicBody,
    BT_DYNAMIC = b2_dynamicBody
};

/// 2D rigid body component.
class DV_API RigidBody2D : public Component
{
    DV_OBJECT(RigidBody2D);

public:
    /// Construct.
    explicit RigidBody2D();
    /// Destruct.
    ~RigidBody2D() override;
    /// Register object factory.
    static void register_object();

    /// Handle enabled/disabled state change.
    void OnSetEnabled() override;

    /// Set body type.
    void SetBodyType(BodyType2D type);
    /// Set mass.
    void SetMass(float mass);
    /// Set inertia.
    void SetInertia(float inertia);
    /// Set mass center.
    void SetMassCenter(const Vector2& center);
    /// Set whether to automatically calculate mass and inertia from collision shapes. Default true.
    void SetUseFixtureMass(bool useFixtureMass);
    /// Set linear damping.
    void SetLinearDamping(float linearDamping);
    /// Set angular damping.
    void SetAngularDamping(float angularDamping);
    /// Set allow sleep.
    void SetAllowSleep(bool allowSleep);
    /// Set fixed rotation.
    void SetFixedRotation(bool fixedRotation);
    /// Set bullet mode.
    void SetBullet(bool bullet);
    /// Set gravity scale.
    void SetGravityScale(float gravityScale);
    /// Set awake.
    void SetAwake(bool awake);
    /// Set linear velocity.
    void SetLinearVelocity(const Vector2& linearVelocity);
    /// Set angular velocity.
    void SetAngularVelocity(float angularVelocity);
    /// Apply force.
    void ApplyForce(const Vector2& force, const Vector2& point, bool wake);
    /// Apply force to center.
    void ApplyForceToCenter(const Vector2& force, bool wake);
    /// Apply Torque.
    void ApplyTorque(float torque, bool wake);
    /// Apply linear impulse.
    void ApplyLinearImpulse(const Vector2& impulse, const Vector2& point, bool wake);
    /// Apply linear impulse to center.
    void ApplyLinearImpulseToCenter(const Vector2& impulse, bool wake);
    /// Apply angular impulse.
    void ApplyAngularImpulse(float impulse, bool wake);

    /// Create body.
    void CreateBody();
    /// Release body.
    void ReleaseBody();

    /// Apply world transform from the Box2D body. Called by PhysicsWorld2D.
    void ApplyWorldTransform();
    /// Apply specified world position & rotation. Called by PhysicsWorld2D.
    void ApplyWorldTransform(const Vector3& newWorldPosition, const Quaternion& newWorldRotation);
    /// Add collision shape.
    void AddCollisionShape2D(CollisionShape2D* collisionShape);
    /// Remove collision shape.
    void RemoveCollisionShape2D(CollisionShape2D* collisionShape);
    /// Add constraint.
    void AddConstraint2D(Constraint2D* constraint);
    /// Remove constraint.
    void RemoveConstraint2D(Constraint2D* constraint);

    /// Return body type.
    BodyType2D GetBodyType() const { return body_ ? (BodyType2D)body_->GetType() : (BodyType2D)bodyDef_.type; }

    /// Return mass.
    float GetMass() const;
    /// Return inertia.
    float GetInertia() const;
    /// Return mass center.
    Vector2 GetMassCenter() const;

    /// Return whether to calculate mass and inertia from collision shapes automatically.
    bool GetUseFixtureMass() const { return useFixtureMass_; }

    /// Return linear damping.
    float GetLinearDamping() const { return body_ ? body_->GetLinearDamping() : bodyDef_.linearDamping; }

    /// Return angular damping.
    float GetAngularDamping() const { return body_ ? body_->GetAngularDamping() : bodyDef_.angularDamping; }

    /// Return allow sleep.
    bool IsAllowSleep() const { return body_ ? body_->IsSleepingAllowed() : bodyDef_.allowSleep; }

    /// Return fixed rotation.
    bool IsFixedRotation() const { return body_ ? body_->IsFixedRotation() : bodyDef_.fixedRotation; }

    /// Return bullet mode.
    bool IsBullet() const { return body_ ? body_->IsBullet() : bodyDef_.bullet; }

    /// Return gravity scale.
    float GetGravityScale() const { return body_ ? body_->GetGravityScale() : bodyDef_.gravityScale; }

    /// Return awake.
    bool IsAwake() const;
    /// Return linear velocity.
    Vector2 GetLinearVelocity() const;
    /// Return angular velocity.
    float GetAngularVelocity() const;

    /// Return Box2D body.
    b2Body* GetBody() const { return body_; }

private:
    /// Handle node being assigned.
    void OnNodeSet(Node* node) override;
    /// Handle scene being assigned.
    void OnSceneSet(Scene* scene) override;
    /// Handle node transform being dirtied.
    void OnMarkedDirty(Node* node) override;

    /// Physics world.
    WeakPtr<PhysicsWorld2D> physicsWorld_;
    /// Box2D body define.
    b2BodyDef bodyDef_;
    /// Box2D mass data.
    b2MassData massData_;
    /// Use fixture mass (calculate mass & inertia from collision shapes automatically).
    bool useFixtureMass_;
    /// Box2D body.
    b2Body* body_;
    /// Collision shapes.
    Vector<WeakPtr<CollisionShape2D>> collisionShapes_;
    /// Constraints.
    Vector<WeakPtr<Constraint2D>> constraints_;
};

}
