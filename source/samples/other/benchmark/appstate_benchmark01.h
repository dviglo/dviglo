// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "appstate_base.h"

class AppState_Benchmark01 : public AppState_Base
{
public:
    DV_OBJECT(AppState_Benchmark01, AppState_Base);

public:
    AppState_Benchmark01(dv::Context* context)
        : AppState_Base(context)
    {
        name_ = "Static Scene";
    }

    void OnEnter() override;
    void OnLeave() override;

    void HandleSceneUpdate(dv::StringHash eventType, dv::VariantMap& eventData);
};
