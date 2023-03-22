// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "collision_shape_2d.h"

namespace dviglo
{

/// 2D edge collision component.
class DV_API CollisionEdge2D : public CollisionShape2D
{
    DV_OBJECT(CollisionEdge2D, CollisionShape2D);

public:
    /// Construct.
    explicit CollisionEdge2D();
    /// Destruct.
    ~CollisionEdge2D() override;
    /// Register object factory.
    static void RegisterObject();

    /// Set vertex 1.
    void SetVertex1(const Vector2& vertex);
    /// Set vertex 2.
    void SetVertex2(const Vector2& vertex);
    /// Set vertices.
    void SetVertices(const Vector2& vertex1, const Vector2& vertex2);

    /// Return vertex 1.
    const Vector2& GetVertex1() const { return vertex1_; }

    /// Return vertex 2.
    const Vector2& GetVertex2() const { return vertex2_; }

private:
    /// Apply node world scale.
    void ApplyNodeWorldScale() override;
    /// Recreate fixture.
    void RecreateFixture();

    /// Edge shape.
    b2EdgeShape edge_shape_;
    /// Vertex 1.
    Vector2 vertex1_;
    /// Vertex 2.
    Vector2 vertex2_;
};

}
