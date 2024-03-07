// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../io/memory_buffer.h"
#include "../io/vector_buffer.h"
#include "collision_chain_2d.h"
#include "physics_utils_2d.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* PHYSICS2D_CATEGORY;

CollisionChain2D::CollisionChain2D() :
    loop_(false)
{
    fixtureDef_.shape = &chain_shape_;
}

CollisionChain2D::~CollisionChain2D() = default;

void CollisionChain2D::register_object()
{
    DV_CONTEXT->RegisterFactory<CollisionChain2D>(PHYSICS2D_CATEGORY);

    DV_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, true, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Loop", GetLoop, SetLoop, false, AM_DEFAULT);
    DV_COPY_BASE_ATTRIBUTES(CollisionShape2D);
    DV_ACCESSOR_ATTRIBUTE("Vertices", GetVerticesAttr, SetVerticesAttr, Variant::emptyBuffer, AM_FILE);
}

void CollisionChain2D::SetLoop(bool loop)
{
    if (loop == loop_)
        return;

    loop_ = loop;

    MarkNetworkUpdate();
    RecreateFixture();
}

void CollisionChain2D::SetVertexCount(i32 count)
{
    assert(count >= 0);
    vertices_.Resize(count);
}

void CollisionChain2D::SetVertex(i32 index, const Vector2& vertex)
{
    assert(index >= 0);

    if (index >= vertices_.Size())
        return;

    vertices_[index] = vertex;

    if (index == vertices_.Size() - 1)
    {
        MarkNetworkUpdate();
        RecreateFixture();
    }
}

void CollisionChain2D::SetVertices(const Vector<Vector2>& vertices)
{
    vertices_ = vertices;

    MarkNetworkUpdate();
    RecreateFixture();
}

void CollisionChain2D::SetVerticesAttr(const Vector<byte>& value)
{
    if (value.Empty())
        return;

    Vector<Vector2> vertices;

    MemoryBuffer buffer(value);
    while (!buffer.IsEof())
        vertices.Push(buffer.ReadVector2());

    SetVertices(vertices);
}

Vector<byte> CollisionChain2D::GetVerticesAttr() const
{
    VectorBuffer ret;

    for (const Vector2& vertex : vertices_)
        ret.WriteVector2(vertex);

    return ret.GetBuffer();
}

void CollisionChain2D::ApplyNodeWorldScale()
{
    RecreateFixture();
}

void CollisionChain2D::RecreateFixture()
{
    ReleaseFixture();

    Vector<b2Vec2> b2Vertices;
    i32 count = vertices_.Size();
    b2Vertices.Resize(count);

    Vector2 worldScale(cachedWorldScale_.x, cachedWorldScale_.y);
    for (i32 i = 0; i < count; ++i)
        b2Vertices[i] = ToB2Vec2(vertices_[i] * worldScale);

    chain_shape_.Clear();

    if (loop_)
    {
        if (count < 2)
            return;

        chain_shape_.CreateLoop(&b2Vertices[0], count);
    }
    else
    {
        if (count < 4)
            return;

        chain_shape_.CreateChain(&b2Vertices[1], count - 2, b2Vertices[0], b2Vertices[count - 1]);
    }

    CreateFixture();
}

}
