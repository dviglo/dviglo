// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include <dviglo/scene/logic_component.h>
#include <dviglo/math/bounding_box.h>

namespace dv = dviglo;

// Custom logic component for moving the animated model and rotating at area edges
class Benchmark02_WomanMover : public dv::LogicComponent
{
public:
    DV_OBJECT(Benchmark02_WomanMover, LogicComponent);

private:
    // Forward movement speed
    float moveSpeed_;
    
    // Rotation speed
    float rotationSpeed_;
    
    // Movement boundaries
    dv::BoundingBox bounds_;

public:
    explicit Benchmark02_WomanMover();

    // Set motion parameters: forward movement speed, rotation speed, and movement boundaries
    void SetParameters(float moveSpeed, float rotationSpeed, const dv::BoundingBox& bounds);
    
    // Handle scene update. Called by LogicComponent base class
    void Update(float timeStep) override;

    // Return forward movement speed
    float GetMoveSpeed() const { return moveSpeed_; }
    
    // Return rotation speed
    float GetRotationSpeed() const { return rotationSpeed_; }
    
    // Return movement boundaries
    const dv::BoundingBox& GetBounds() const { return bounds_; }
};
