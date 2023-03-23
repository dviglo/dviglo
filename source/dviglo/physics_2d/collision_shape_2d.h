// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../scene/component.h"

#include <box2d/box2d.h>

namespace dviglo
{

class RigidBody2D;

/// 2D collision shape component.
class DV_API CollisionShape2D : public Component
{
    DV_OBJECT(CollisionShape2D, Component);

public:
    /// Construct.
    explicit CollisionShape2D();
    /// Destruct.
    ~CollisionShape2D() override;
    /// Register object factory.
    static void register_object();

    /// Handle enabled/disabled state change.
    void OnSetEnabled() override;

    /// Set trigger.
    void SetTrigger(bool trigger);
    /// Set filter category bits.
    void SetCategoryBits(int categoryBits);
    /// Set filter mask bits.
    void SetMaskBits(int maskBits);
    /// Set filter group index.
    void SetGroupIndex(int groupIndex);
    /// Set density.
    void SetDensity(float density);
    /// Set friction.
    void SetFriction(float friction);
    /// Set restitution.
    void SetRestitution(float restitution);

    /// Create fixture.
    void CreateFixture();
    /// Release fixture.
    void ReleaseFixture();

    /// Return trigger.
    bool IsTrigger() const { return fixtureDef_.isSensor; }

    /// Return filter category bits.
    int GetCategoryBits() const { return fixtureDef_.filter.categoryBits; }

    /// Return filter mask bits.
    int GetMaskBits() const { return fixtureDef_.filter.maskBits; }

    /// Return filter group index.
    int GetGroupIndex() const { return fixtureDef_.filter.groupIndex; }

    /// Return density.
    float GetDensity() const { return fixtureDef_.density; }

    /// Return friction.
    float GetFriction() const { return fixtureDef_.friction; }

    /// Return restitution.
    float GetRestitution() const { return fixtureDef_.restitution; }

    /// Return mass.
    float GetMass() const;
    /// Return inertia.
    float GetInertia() const;
    /// Return mass center.
    Vector2 GetMassCenter() const;

    /// Return fixture.
    b2Fixture* GetFixture() const { return fixture_; }

protected:
    /// Handle node being assigned.
    void OnNodeSet(Node* node) override;
    /// Handle node transform being dirtied.
    void OnMarkedDirty(Node* node) override;
    /// Apply Node world scale.
    virtual void ApplyNodeWorldScale() = 0;

    /// Rigid body.
    WeakPtr<RigidBody2D> rigidBody_;
    /// Fixture def.
    b2FixtureDef fixtureDef_;
    /// Box2D fixture.
    b2Fixture* fixture_;
    /// Cached world scale.
    Vector3 cachedWorldScale_;
};

}
