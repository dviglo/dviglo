// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "collision_shape_2d.h"

namespace dviglo
{

/// 2D box collision component.
class DV_API CollisionBox2D : public CollisionShape2D
{
    DV_OBJECT(CollisionBox2D, CollisionShape2D);

public:
    /// Construct.
    explicit CollisionBox2D();
    /// Destruct.
    ~CollisionBox2D() override;
    /// Register object factory.
    static void RegisterObject();

    /// Set size.
    void SetSize(const Vector2& size);
    /// Set size.
    void SetSize(float width, float height);
    /// Set center.
    void SetCenter(const Vector2& center);
    /// Set center.
    void SetCenter(float x, float y);
    /// Set angle.
    void SetAngle(float angle);

    /// Return size.
    const Vector2& GetSize() const { return size_; }

    /// Return center.
    const Vector2& GetCenter() const { return center_; }

    /// Return angle.
    float GetAngle() const { return angle_; }

private:
    /// Apply node world scale.
    void ApplyNodeWorldScale() override;
    /// Recreate fixture.
    void RecreateFixture();

    /// Box shape.
    b2PolygonShape boxShape_;
    /// Size.
    Vector2 size_;
    /// Center.
    Vector2 center_;
    /// Angle.
    float angle_;
};

}
