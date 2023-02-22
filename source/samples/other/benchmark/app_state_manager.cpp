// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "app_state_manager.h"

#include "appstate_benchmark01.h"
#include "appstate_benchmark02.h"
#include "appstate_benchmark03.h"
#include "appstate_benchmark04.h"
#include "appstate_main_screen.h"
#include "appstate_result_screen.h"

#include <dviglo/common/debug_new.h>

using namespace dviglo;

AppStateManager::AppStateManager()
{
    appStates_.Insert({APPSTATEID_MAINSCREEN, MakeShared<AppState_MainScreen>()});
    appStates_.Insert({APPSTATEID_RESULTSCREEN, MakeShared<AppState_ResultScreen>()});
    appStates_.Insert({APPSTATEID_BENCHMARK01, MakeShared<AppState_Benchmark01>()});
    appStates_.Insert({APPSTATEID_BENCHMARK02, MakeShared<AppState_Benchmark02>()});
    appStates_.Insert({APPSTATEID_BENCHMARK03, MakeShared<AppState_Benchmark03>()});
    appStates_.Insert({APPSTATEID_BENCHMARK04, MakeShared<AppState_Benchmark04>()});
}

void AppStateManager::Apply()
{
    if (requiredAppStateId_ == currentAppStateId_)
        return;

    assert(requiredAppStateId_ != APPSTATEID_NULL);

    if (currentAppStateId_ != APPSTATEID_NULL)
    {
        SharedPtr<AppState_Base> currentAppStatePtr = appStates_[currentAppStateId_];
        currentAppStatePtr->OnLeave();
    }

    previousAppStateId_ = currentAppStateId_;
    currentAppStateId_ = requiredAppStateId_;

    SharedPtr<AppState_Base> requiredAppStatePtr = appStates_[requiredAppStateId_];
    requiredAppStatePtr->OnEnter();
}
