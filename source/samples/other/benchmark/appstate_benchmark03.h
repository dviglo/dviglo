// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "appstate_base.h"

// Huge number of interactions between components
class AppState_Benchmark03 : public AppState_Base
{
public:
    DV_OBJECT(AppState_Benchmark03);

public:
    AppState_Benchmark03();

    void OnEnter() override;
    void OnLeave() override;

    void CreateMolecule(const Vector2& pos, i32 type);

    void HandleSceneUpdate(StringHash eventType, VariantMap& eventData);
};
