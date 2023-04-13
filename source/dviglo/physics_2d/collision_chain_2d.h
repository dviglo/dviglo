// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "collision_shape_2d.h"

namespace dviglo
{
/// 2D chain collision component.
class DV_API CollisionChain2D : public CollisionShape2D
{
    DV_OBJECT(CollisionChain2D);

public:
    /// Construct.
    explicit CollisionChain2D();

    /// Destruct.
    ~CollisionChain2D() override;

    /// Register object factory.
    static void register_object();

    /// Set loop.
    void SetLoop(bool loop);

    /// Return loop.
    bool GetLoop() const { return loop_; }

    /// Set vertex count.
    void SetVertexCount(i32 count);

    /// Return vertex count.
    i32 GetVertexCount() const { return vertices_.Size(); }

    /// Set vertex.
    void SetVertex(i32 index, const Vector2& vertex);

    /// Return vertex.
    const Vector2& GetVertex(i32 index) const
    {
        assert(index >= 0);
        return (index < vertices_.Size()) ? vertices_[index] : Vector2::ZERO;
    }

    /// Set vertices. For non loop first and last must be ghost.
    void SetVertices(const Vector<Vector2>& vertices);

    /// Return vertices.
    const Vector<Vector2>& GetVertices() const { return vertices_; }

    /// Set vertices attribute. For non loop first and last must be ghost.
    void SetVerticesAttr(const Vector<byte>& value);

    /// Return vertices attribute.
    Vector<byte> GetVerticesAttr() const;

private:
    /// Apply node world scale.
    void ApplyNodeWorldScale() override;

    /// Recreate fixture.
    void RecreateFixture();

    /// Chain shape.
    b2ChainShape chain_shape_;

    /// Loop.
    bool loop_;

    /// Vertices.
    Vector<Vector2> vertices_;
};

}
