// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "fps_counter.h"

#include <dviglo/scene/scene.h>

namespace dv = dviglo;

inline const dv::String CURRENT_FPS_STR = "Current FPS";

class AppState_Base : public dv::Object
{
public:
    URHO3D_OBJECT(AppState_Base, Object);

protected:
    dv::String name_ = "Название бенчмарка";

    dv::SharedPtr<dv::Scene> scene_;
    void LoadSceneXml(const dv::String& path);

    FpsCounter fpsCounter_;
    void UpdateCurrentFpsElement();
    
    void SetupViewport();
    void DestroyViewport();

public:
    const dv::String& GetName() const { return name_; }
    const FpsCounter& GetResult() const { return fpsCounter_; }

    AppState_Base(dv::Context* context)
        : Object(context)
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
