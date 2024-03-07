// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "collision_shape_2d.h"

namespace dviglo
{

/// 2D circle collision component.
class DV_API CollisionCircle2D : public CollisionShape2D
{
    DV_OBJECT(CollisionCircle2D);

public:
    /// Construct.
    explicit CollisionCircle2D();
    /// Destruct.
    ~CollisionCircle2D() override;
    /// Register object factory.
    static void register_object();

    /// Set radius.
    void SetRadius(float radius);
    /// Set center.
    void SetCenter(const Vector2& center);
    /// Set center.
    void SetCenter(float x, float y);

    /// Return radius.
    float GetRadius() const { return radius_; }

    /// Return center.
    const Vector2& GetCenter() const { return center_; }

private:
    /// Apply node world scale.
    void ApplyNodeWorldScale() override;
    /// Recreate fixture.
    void RecreateFixture();

    /// Circle shape.
    b2CircleShape circle_shape_;
    /// Radius.
    float radius_;
    /// Center.
    Vector2 center_;
};

}
