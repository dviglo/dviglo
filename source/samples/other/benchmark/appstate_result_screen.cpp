// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "appstate_result_screen.h"
#include "app_state_manager.h"

#include <dviglo/input/input.h>
#include <dviglo/scene/scene_events.h>
#include <dviglo/ui/button.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>
#include <dviglo/ui/ui_events.h>
#include <dviglo/ui/window.h>

#include <dviglo/common/debug_new.h>

static const String RESULT_WINDOW_STR = "Result Window";

void AppState_ResultScreen::OnEnter()
{
    assert(!scene_);
    LoadSceneXml("benchmark/scenes/result_screen.xml");

    DV_INPUT.SetMouseVisible(true);
    setup_viewport();
    subscribe_to_event(scene_, E_SCENEUPDATE, DV_HANDLER(AppState_ResultScreen, HandleSceneUpdate));
    fpsCounter_.Clear();
    ShowResultWindow();
}

void AppState_ResultScreen::OnLeave()
{
    DestroyViewport();
    DestroyResultWindow();
    scene_ = nullptr;
}

void AppState_ResultScreen::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
{
    float timeStep = eventData[SceneUpdate::P_TIMESTEP].GetFloat();

    fpsCounter_.Update(timeStep);
    UpdateCurrentFpsElement();

    if (DV_INPUT.GetKeyDown(KEY_ESCAPE) || DV_INPUT.GetKeyDown(KEY_RETURN) || DV_INPUT.GetKeyDown(KEY_KP_ENTER))
        APP_STATE_MANAGER.SetRequiredAppStateId(APPSTATEID_MAINSCREEN);
}

void AppState_ResultScreen::ShowResultWindow()
{
    UiElement* root = DV_UI->GetRoot();

    Window* window = root->create_child<Window>(RESULT_WINDOW_STR);
    window->SetStyleAuto();
    window->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
    window->SetAlignment(HA_CENTER, VA_CENTER);

    Text* windowTitle = window->create_child<Text>();
    windowTitle->SetStyleAuto();
    windowTitle->SetText("Result");

    AppStateId prevAppStateId = APP_STATE_MANAGER.GetPreviousAppStateId();
    const String& benchmarkName = APP_STATE_MANAGER.GetName(prevAppStateId);
    const FpsCounter& benchmarkResult = APP_STATE_MANAGER.GetResult(prevAppStateId);

    Text* resultText = window->create_child<Text>();
    resultText->SetStyleAuto();
    resultText->SetText(benchmarkName + ": " + String(benchmarkResult.GetResultFps()) + " FPS (min: " +
        String(benchmarkResult.GetResultMinFps()) + ", max: " + String(benchmarkResult.GetResultMaxFps()) + ")");

    Button* okButton = window->create_child<Button>();
    okButton->SetStyleAuto();
    okButton->SetFixedHeight(24);

    Text* buttonText = okButton->create_child<Text>();
    buttonText->SetStyleAuto();
    buttonText->SetText("Ok");
    buttonText->SetAlignment(HA_CENTER, VA_CENTER);

    subscribe_to_event(okButton, E_RELEASED, DV_HANDLER(AppState_ResultScreen, HandleResultOkButtonPressed));
}

void AppState_ResultScreen::DestroyResultWindow()
{
    UiElement* root = DV_UI->GetRoot();
    UiElement* window = root->GetChild(RESULT_WINDOW_STR);
    window->Remove();
}

void AppState_ResultScreen::HandleResultOkButtonPressed(StringHash eventType, VariantMap& eventData)
{
    APP_STATE_MANAGER.SetRequiredAppStateId(APPSTATEID_MAINSCREEN);
}
