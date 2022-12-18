// Copyright (c) 2008-2022 the Urho3D project
// Copyright (c) 2022-2022 the Dviglo project
// License: MIT

/// \file
/// @nobindfile

#pragma once

#include "../math/color.h"
#include "../math/vector2.h"
#include "../math/vector3.h"

#include <box2d/box2d.h>

namespace Urho3D
{

inline Color ToColor(const b2Color& color)
{
    return Color(color.r, color.g, color.b);
}

inline b2Vec2 ToB2Vec2(const Vector2& vector)
{
    return {vector.x_, vector.y_};
}

inline Vector2 ToVector2(const b2Vec2& vec2)
{
    return Vector2(vec2.x, vec2.y);
}

inline b2Vec2 ToB2Vec2(const Vector3& vector)
{
    return {vector.x_, vector.y_};
}

inline Vector3 ToVector3(const b2Vec2& vec2)
{
    return Vector3(vec2.x, vec2.y, 0.0f);
}

}
