// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "collision_edge_2d.h"
#include "physics_utils_2d.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* PHYSICS2D_CATEGORY;
static const Vector2 DEFAULT_VERTEX1(-0.01f, 0.0f);
static const Vector2 DEFAULT_VERTEX2(0.01f, 0.0f);

CollisionEdge2D::CollisionEdge2D() :
    vertex1_(DEFAULT_VERTEX1),
    vertex2_(DEFAULT_VERTEX2)
{
    Vector2 worldScale(cachedWorldScale_.x_, cachedWorldScale_.y_);
    edgeShape_.SetTwoSided(ToB2Vec2(vertex1_ * worldScale), ToB2Vec2(vertex2_ * worldScale));

    fixtureDef_.shape = &edgeShape_;
}

CollisionEdge2D::~CollisionEdge2D() = default;

void CollisionEdge2D::RegisterObject()
{
    DV_CONTEXT.RegisterFactory<CollisionEdge2D>(PHYSICS2D_CATEGORY);

    DV_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, true, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Vertex 1", GetVertex1, SetVertex1, DEFAULT_VERTEX1, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Vertex 2", GetVertex2, SetVertex2, DEFAULT_VERTEX2, AM_DEFAULT);
    DV_COPY_BASE_ATTRIBUTES(CollisionShape2D);
}

void CollisionEdge2D::SetVertex1(const Vector2& vertex)
{
    SetVertices(vertex, vertex2_);
}

void CollisionEdge2D::SetVertex2(const Vector2& vertex)
{
    SetVertices(vertex1_, vertex);
}

void CollisionEdge2D::SetVertices(const Vector2& vertex1, const Vector2& vertex2)
{
    if (vertex1 == vertex1_ && vertex2 == vertex2_)
        return;

    vertex1_ = vertex1;
    vertex2_ = vertex2;

    MarkNetworkUpdate();
    RecreateFixture();
}

void CollisionEdge2D::ApplyNodeWorldScale()
{
    RecreateFixture();
}

void CollisionEdge2D::RecreateFixture()
{
    ReleaseFixture();

    Vector2 worldScale(cachedWorldScale_.x_, cachedWorldScale_.y_);
    edgeShape_.SetTwoSided(ToB2Vec2(vertex1_ * worldScale), ToB2Vec2(vertex2_ * worldScale));

    CreateFixture();
}

}
