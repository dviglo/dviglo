// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "appstate_base.h"

class AppState_ResultScreen : public AppState_Base
{
public:
    DV_OBJECT(AppState_ResultScreen);

public:
    AppState_ResultScreen()
    {
        name_ = "Result Screen";
    }

    void OnEnter() override;
    void OnLeave() override;

    void HandleSceneUpdate(StringHash eventType, VariantMap& eventData);

    void ShowResultWindow();
    void DestroyResultWindow();
    void HandleResultOkButtonPressed(StringHash eventType, VariantMap& eventData);
};
