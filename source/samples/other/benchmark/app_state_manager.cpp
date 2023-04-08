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

#include <dviglo/io/log.h>

#include <dviglo/common/debug_new.h>

using namespace dviglo;

#ifndef NDEBUG
// Проверяем, что не происходит обращения к синглтону после вызова деструктора
static bool app_state_manager_destructed = false;
#endif

// Определение должно быть в cpp-файле, иначе будут проблемы в shared-версии движка в MinGW.
// Когда функция в h-файле, в exe и в dll создаются свои экземпляры объекта с разными адресами.
// https://stackoverflow.com/questions/71830151/why-singleton-in-headers-not-work-for-windows-mingw
AppStateManager& AppStateManager::get_instance()
{
    assert(!app_state_manager_destructed);
    static AppStateManager instance;
    return instance;
}

AppStateManager::AppStateManager()
{
    appStates_.Insert({APPSTATEID_MAINSCREEN, MakeShared<AppState_MainScreen>()});
    appStates_.Insert({APPSTATEID_RESULTSCREEN, MakeShared<AppState_ResultScreen>()});
    appStates_.Insert({APPSTATEID_BENCHMARK01, MakeShared<AppState_Benchmark01>()});
    appStates_.Insert({APPSTATEID_BENCHMARK02, MakeShared<AppState_Benchmark02>()});
    appStates_.Insert({APPSTATEID_BENCHMARK03, MakeShared<AppState_Benchmark03>()});
    appStates_.Insert({APPSTATEID_BENCHMARK04, MakeShared<AppState_Benchmark04>()});

    DV_LOGDEBUG("Singleton AppStateManager constructed");
}

AppStateManager::~AppStateManager()
{
    DV_LOGDEBUG("Singleton AppStateManager destructed");

#ifndef NDEBUG
    app_state_manager_destructed = true;
#endif
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
