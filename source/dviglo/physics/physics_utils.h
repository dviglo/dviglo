// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

/// \file

#pragma once

#include "../math/quaternion.h"
#include "../math/vector3.h"

#include <bullet/LinearMath/btVector3.h>
#include <bullet/LinearMath/btQuaternion.h>

namespace Urho3D
{

inline btVector3 ToBtVector3(const Vector3& vector)
{
    return btVector3(vector.x_, vector.y_, vector.z_);
}

inline btQuaternion ToBtQuaternion(const Quaternion& quaternion)
{
    return btQuaternion(quaternion.x_, quaternion.y_, quaternion.z_, quaternion.w_);
}

inline Vector3 ToVector3(const btVector3& vector)
{
    return Vector3(vector.x(), vector.y(), vector.z());
}

inline Quaternion ToQuaternion(const btQuaternion& quaternion)
{
    return Quaternion(quaternion.w(), quaternion.x(), quaternion.y(), quaternion.z());
}

inline bool HasWorldScaleChanged(const Vector3& oldWorldScale, const Vector3& newWorldScale)
{
    Vector3 delta = newWorldScale - oldWorldScale;
    float dot = delta.DotProduct(delta);
    return dot > 0.01f;
}

}
