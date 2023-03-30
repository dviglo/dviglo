// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "appstate_benchmark01.h"
#include "app_state_manager.h"

#include <dviglo/input/input.h>
#include <dviglo/scene/scene_events.h>

#include <dviglo/common/debug_new.h>

using namespace dviglo;

void AppState_Benchmark01::OnEnter()
{
    assert(!scene_);
    LoadSceneXml("benchmark/scenes/benchmark01.xml");

    DV_INPUT.SetMouseVisible(false);
    setup_viewport();
    subscribe_to_event(scene_, E_SCENEUPDATE, DV_HANDLER(AppState_Benchmark01, HandleSceneUpdate));
    fpsCounter_.Clear();
}

void AppState_Benchmark01::OnLeave()
{
    UnsubscribeFromAllEvents();
    DestroyViewport();
    scene_ = nullptr;
}

void AppState_Benchmark01::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
{
    float timeStep = eventData[SceneUpdate::P_TIMESTEP].GetFloat();

    fpsCounter_.Update(timeStep);
    UpdateCurrentFpsElement();

    if (DV_INPUT.GetKeyDown(KEY_ESCAPE))
    {
        APP_STATE_MANAGER.SetRequiredAppStateId(APPSTATEID_MAINSCREEN);
        return;
    }

    if (fpsCounter_.GetTotalTime() >= 25.f)
        APP_STATE_MANAGER.SetRequiredAppStateId(APPSTATEID_RESULTSCREEN);
}
