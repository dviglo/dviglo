// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "appstate_base.h"

class AppState_Benchmark02 : public AppState_Base
{
public:
    DV_OBJECT(AppState_Benchmark02);

public:
    AppState_Benchmark02();

    void OnEnter() override;
    void OnLeave() override;

    void HandleSceneUpdate(StringHash eventType, VariantMap& eventData);
};
