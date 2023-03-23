// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "collision_circle_2d.h"
#include "physics_utils_2d.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* PHYSICS2D_CATEGORY;
static const float DEFAULT_CLRCLE_RADIUS(0.01f);

CollisionCircle2D::CollisionCircle2D() :
    radius_(DEFAULT_CLRCLE_RADIUS),
    center_(Vector2::ZERO)
{
    circle_shape_.m_radius = DEFAULT_CLRCLE_RADIUS * cachedWorldScale_.x_;
    fixtureDef_.shape = &circle_shape_;
}

CollisionCircle2D::~CollisionCircle2D() = default;

void CollisionCircle2D::register_object()
{
    DV_CONTEXT.RegisterFactory<CollisionCircle2D>(PHYSICS2D_CATEGORY);

    DV_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, true, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Radius", GetRadius, SetRadius, DEFAULT_CLRCLE_RADIUS, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Center", GetCenter, SetCenter, Vector2::ZERO, AM_DEFAULT);
    DV_COPY_BASE_ATTRIBUTES(CollisionShape2D);
}

void CollisionCircle2D::SetRadius(float radius)
{
    if (radius == radius_)
        return;

    radius_ = radius;

    RecreateFixture();
    MarkNetworkUpdate();
}

void CollisionCircle2D::SetCenter(const Vector2& center)
{
    if (center == center_)
        return;

    center_ = center;

    RecreateFixture();
    MarkNetworkUpdate();
}

void CollisionCircle2D::SetCenter(float x, float y)
{
    SetCenter(Vector2(x, y));
}

void CollisionCircle2D::ApplyNodeWorldScale()
{
    RecreateFixture();
}

void CollisionCircle2D::RecreateFixture()
{
    ReleaseFixture();

    // Only use scale in x axis for circle
    float worldScale = cachedWorldScale_.x_;
    circle_shape_.m_radius = radius_ * worldScale;
    circle_shape_.m_p = ToB2Vec2(center_ * worldScale);

    CreateFixture();
}

}
