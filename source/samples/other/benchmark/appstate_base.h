// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "fps_counter.h"

#include <dviglo/scene/scene.h>

using namespace dviglo;

inline const String CURRENT_FPS_STR = "Current FPS";

class AppState_Base : public Object
{
public:
    DV_OBJECT(AppState_Base);

protected:
    String name_ = "Название бенчмарка";

    SharedPtr<Scene> scene_;
    void LoadSceneXml(const String& path);

    FpsCounter fpsCounter_;
    void UpdateCurrentFpsElement();
    
    void setup_viewport();
    void DestroyViewport();

public:
    const String& GetName() const { return name_; }
    const FpsCounter& GetResult() const { return fpsCounter_; }

    AppState_Base()
    {
    }

    void ClearResult() { fpsCounter_.Clear(); }

    virtual void OnEnter()
    {
    }

    virtual void OnLeave()
    {
    }
};

// Note: scene_ and GUI are destroyed and recreated on every state change so as not to affect other benchmarks
