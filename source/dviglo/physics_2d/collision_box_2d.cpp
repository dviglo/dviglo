// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "collision_box_2d.h"
#include "physics_utils_2d.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* PHYSICS2D_CATEGORY;
static const Vector2 DEFAULT_BOX_SIZE(0.01f, 0.01f);

CollisionBox2D::CollisionBox2D() :
    size_(DEFAULT_BOX_SIZE),
    center_(Vector2::ZERO),
    angle_(0.0f)
{
    float halfWidth = size_.x_ * 0.5f * cachedWorldScale_.x;
    float halfHeight = size_.y_ * 0.5f * cachedWorldScale_.y;
    box_shape_.SetAsBox(halfWidth, halfHeight);
    fixtureDef_.shape = &box_shape_;
}

CollisionBox2D::~CollisionBox2D() = default;

void CollisionBox2D::register_object()
{
    DV_CONTEXT.RegisterFactory<CollisionBox2D>(PHYSICS2D_CATEGORY);

    DV_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, true, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Size", GetSize, SetSize, DEFAULT_BOX_SIZE, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Center", GetCenter, SetCenter, Vector2::ZERO, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Angle", GetAngle, SetAngle, 0.0f, AM_DEFAULT);
    DV_COPY_BASE_ATTRIBUTES(CollisionShape2D);
}

void CollisionBox2D::SetSize(const Vector2& size)
{
    if (size == size_)
        return;

    size_ = size;

    MarkNetworkUpdate();
    RecreateFixture();
}

void CollisionBox2D::SetSize(float width, float height)
{
    SetSize(Vector2(width, height));
}

void CollisionBox2D::SetCenter(const Vector2& center)
{
    if (center == center_)
        return;

    center_ = center;

    MarkNetworkUpdate();
    RecreateFixture();
}

void CollisionBox2D::SetCenter(float x, float y)
{
    SetCenter(Vector2(x, y));
}

void CollisionBox2D::SetAngle(float angle)
{
    if (angle == angle_)
        return;

    angle_ = angle;

    MarkNetworkUpdate();
    RecreateFixture();
}

void CollisionBox2D::ApplyNodeWorldScale()
{
    RecreateFixture();
}

void CollisionBox2D::RecreateFixture()
{
    ReleaseFixture();

    float worldScaleX = cachedWorldScale_.x;
    float worldScaleY = cachedWorldScale_.y;
    float halfWidth = size_.x_ * 0.5f * worldScaleX;
    float halfHeight = size_.y_ * 0.5f * worldScaleY;
    Vector2 scaledCenter = center_ * Vector2(worldScaleX, worldScaleY);

    if (scaledCenter == Vector2::ZERO && angle_ == 0.0f)
        box_shape_.SetAsBox(halfWidth, halfHeight);
    else
        box_shape_.SetAsBox(halfWidth, halfHeight, ToB2Vec2(scaledCenter), angle_ * M_DEGTORAD);

    CreateFixture();
}

}
