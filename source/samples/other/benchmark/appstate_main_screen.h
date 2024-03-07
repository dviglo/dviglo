// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
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
    DV_OBJECT(AppState_MainScreen);

    SlotSceneUpdate scene_update;

private:
    void HandleButtonPressed(StringHash eventType, VariantMap& eventData);
    void CreateButton(const String& name, const String& text, Window& parent);
    void CreateGui();
    void DestroyGui();

public:
    AppState_MainScreen()
    {
        name_ = "Main Screen";
    }

    void OnEnter() override;
    void OnLeave() override;

    void handle_scene_update(Scene* scene, float time_step);
};
