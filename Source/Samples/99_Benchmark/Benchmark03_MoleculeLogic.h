// Copyright (c) 2008-2022 the Urho3D project
// Copyright (c) 2022-2022 the Dviglo project
// License: MIT

#pragma once

#include <dviglo/scene/logic_component.h>
#include <dviglo/base/primitive_types.h>

namespace U3D = Urho3D;
using namespace Urho3D::PrimitiveTypes;

class Benchmark03_MoleculeLogic : public U3D::LogicComponent
{
public:
    URHO3D_OBJECT(Benchmark03_MoleculeLogic, LogicComponent);

private:
    i32 moleculeType_;
    U3D::Vector2 velocity_;
    U3D::Vector2 force_;

public:
    explicit Benchmark03_MoleculeLogic(U3D::Context* context);

    void SetParameters(i32 moleculeType);

    i32 GetMoleculeType() const { return moleculeType_; }

    // Update the velocity of this molecule
    void Update(float timeStep) override;

    // Move this molecule only after updating the velocities of all molecules 
    void PostUpdate(float timeStep) override;
};
