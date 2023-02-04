// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

/// \file

#pragma once

#include "../math/quaternion.h"
#include "../math/vector3.h"

#include <ik/quat.h>
#include <ik/vec3.h>

namespace Urho3D {

/// Converts from an Urho3D Vector3 to an IK vec3_t.
vec3_t Vec3Urho2IK(const Vector3& urho);
/// Converts from an IK vec3_t to an Urho3D Vector3.
Vector3 Vec3IK2Urho(const vec3_t* ik);
/// Converts from an Urho3D quaternion to an IK quat_t.
quat_t QuatUrho2IK(const Quaternion& urho);
/// Converts from an IK quat_t to an Urho3D quaternion.
Quaternion QuatIK2Urho(const quat_t* ik);

}
