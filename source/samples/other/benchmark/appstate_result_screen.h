// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "appstate_base.h"

class AppState_ResultScreen : public AppState_Base
{
public:
    DV_OBJECT(AppState_ResultScreen, AppState_Base);

public:
    AppState_ResultScreen()
    {
        name_ = "Result Screen";
    }

    void OnEnter() override;
    void OnLeave() override;

    void HandleSceneUpdate(dv::StringHash eventType, dv::VariantMap& eventData);

    void ShowResultWindow();
    void DestroyResultWindow();
    void HandleResultOkButtonPressed(dv::StringHash eventType, dv::VariantMap& eventData);
};
