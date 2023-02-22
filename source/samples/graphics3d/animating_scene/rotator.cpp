// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/scene/scene.h>

#include "rotator.h"

#include <dviglo/common/debug_new.h>

Rotator::Rotator() :
    rotationSpeed_(Vector3::ZERO)
{
    // Only the scene update event is needed: unsubscribe from the rest for optimization
    SetUpdateEventMask(LogicComponentEvents::Update);
}

void Rotator::SetRotationSpeed(const Vector3& speed)
{
    rotationSpeed_ = speed;
}

void Rotator::Update(float timeStep)
{
    // Components have their scene node as a member variable for convenient access. Rotate the scene node now: construct a
    // rotation quaternion from Euler angles, scale rotation speed with the scene update time step
    node_->Rotate(Quaternion(rotationSpeed_.x_ * timeStep, rotationSpeed_.y_ * timeStep, rotationSpeed_.z_ * timeStep));
}
