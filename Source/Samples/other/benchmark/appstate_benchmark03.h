// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "appstate_base.h"

// Huge number of interactions between components
class AppState_Benchmark03 : public AppState_Base
{
public:
    URHO3D_OBJECT(AppState_Benchmark03, AppState_Base);

public:
    AppState_Benchmark03(U3D::Context* context);

    void OnEnter() override;
    void OnLeave() override;

    void CreateMolecule(const U3D::Vector2& pos, i32 type);

    void HandleSceneUpdate(U3D::StringHash eventType, U3D::VariantMap& eventData);
};
