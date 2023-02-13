// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include <dviglo/scene/logic_component.h>
#include <dviglo/base/primitive_types.h>

namespace dv = dviglo;
using namespace dviglo::PrimitiveTypes;

class Benchmark03_MoleculeLogic : public dv::LogicComponent
{
public:
    URHO3D_OBJECT(Benchmark03_MoleculeLogic, LogicComponent);

private:
    i32 moleculeType_;
    dv::Vector2 velocity_;
    dv::Vector2 force_;

public:
    explicit Benchmark03_MoleculeLogic(dv::Context* context);

    void SetParameters(i32 moleculeType);

    i32 GetMoleculeType() const { return moleculeType_; }

    // Update the velocity of this molecule
    void Update(float timeStep) override;

    // Move this molecule only after updating the velocities of all molecules 
    void PostUpdate(float timeStep) override;
};
