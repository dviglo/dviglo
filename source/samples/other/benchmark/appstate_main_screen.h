// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "appstate_base.h"

namespace dviglo
{
    class Window;
}

class AppState_MainScreen : public AppState_Base
{
public:
    URHO3D_OBJECT(AppState_MainScreen, AppState_Base);

private:
    void HandleButtonPressed(dv::StringHash eventType, dv::VariantMap& eventData);
    void CreateButton(const dv::String& name, const dv::String& text, dv::Window& parent);
    void CreateGui();
    void DestroyGui();

public:
    AppState_MainScreen(dv::Context* context)
        : AppState_Base(context)
    {
        name_ = "Main Screen";
    }

    void OnEnter() override;
    void OnLeave() override;

    void HandleSceneUpdate(dv::StringHash eventType, dv::VariantMap& eventData);
};
