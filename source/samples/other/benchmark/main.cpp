// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "app_state_manager.h"

#include <dviglo/core/core_events.h>
#include <dviglo/engine/application.h>
#include <dviglo/engine/engine_defs.h>
#include <dviglo/input/input.h>
#include <dviglo/io/file_system.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

using namespace dviglo;

class App : public Application
{
    DV_OBJECT(App, Application);

public:
    App(Context* context)
        : Application(context)
    {
        // The first handler for the first event in each frame.
        // To prevent a crash, we can only change the current scene at the start of a frame,
        // before any scene events are processed
        SubscribeToEvent(E_BEGINFRAME, DV_HANDLER(App, ApplyAppState));
    }

    void Setup() override
    {
        engineParameters_[EP_WINDOW_TITLE] = "Urho3D Benchmark";
        engineParameters_[EP_LOG_NAME] = GetSubsystem<FileSystem>()->GetAppPreferencesDir("urho3d", "logs") + "99_Benchmark.log";
        engineParameters_[EP_FULL_SCREEN] = false;
        engineParameters_[EP_WINDOW_WIDTH] = 960;
        engineParameters_[EP_WINDOW_HEIGHT] = 720;
        engineParameters_[EP_FRAME_LIMITER] = false;
    }

    // This elements can be used anywhere in the program
    void CreateCurrentFpsUiElement()
    {
        UIElement* root = GetSubsystem<UI>()->GetRoot();
        root->SetDefaultStyle(GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml"));

        Text* fpsElement = root->CreateChild<Text>(CURRENT_FPS_STR);
        fpsElement->SetStyleAuto();
        fpsElement->SetTextEffect(TE_SHADOW);
        fpsElement->SetPosition(10, 10);
        fpsElement->SetText("FPS");
    }

    void Start() override
    {
        context_->RegisterSubsystem(new AppStateManager(context_));
        AppStateManager* appStateManager = context_->GetSubsystem<AppStateManager>();
        appStateManager->SetRequiredAppStateId(APPSTATEID_MAINSCREEN);

        GetSubsystem<Input>()->SetToggleFullscreen(false); // Block Alt+Enter

        CreateCurrentFpsUiElement();
    }

    void ApplyAppState(StringHash eventType, VariantMap& eventData)
    {
        context_->GetSubsystem<AppStateManager>()->Apply();
    }
};

DV_DEFINE_APPLICATION_MAIN(App);
