// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "benchmark02_woman_mover.h"

#include <dviglo/graphics/animated_model.h>
#include <dviglo/graphics/animation_state.h>
#include <dviglo/scene/scene.h>

#include <dviglo/common/debug_new.h>

using namespace dviglo;

Benchmark02_WomanMover::Benchmark02_WomanMover()
    : moveSpeed_(0.f)
    , rotationSpeed_(0.f)
{
    // Only the scene update event is needed: unsubscribe from the rest for optimization
    SetUpdateEventMask(LogicComponentEvents::Update);
}

void Benchmark02_WomanMover::SetParameters(float moveSpeed, float rotationSpeed, const BoundingBox& bounds)
{
    moveSpeed_ = moveSpeed;
    rotationSpeed_ = rotationSpeed;
    bounds_ = bounds;
}

void Benchmark02_WomanMover::Update(float timeStep)
{
    node_->Translate(Vector3::FORWARD * moveSpeed_ * timeStep);

    // If in risk of going outside the plane, rotate the model right
    Vector3 pos = node_->GetPosition();
    if (pos.x_ < bounds_.min_.x_ || pos.x_ > bounds_.max_.x_ || pos.z_ < bounds_.min_.z_ || pos.z_ > bounds_.max_.z_)
        node_->Yaw(rotationSpeed_ * timeStep);

    // Get the model's first (only) animation state and advance its time. Note the convenience accessor to other components
    // in the same scene node
    AnimatedModel* model = node_->GetComponent<AnimatedModel>(true);
    if (model->GetNumAnimationStates())
    {
        AnimationState* state = model->GetAnimationStates()[0];
        state->AddTime(timeStep);
    }
}
