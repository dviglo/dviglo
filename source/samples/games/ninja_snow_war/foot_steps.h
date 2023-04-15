// Copyright (c) 2022-2023 the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#pragma once

#include <dviglo/dviglo_all.h>

using namespace dviglo;

class FootSteps : public LogicComponent
{
    DV_OBJECT(FootSteps);

public:
    static void register_object();

    FootSteps();

    void Start() override;
    void HandleAnimationTrigger(StringHash eventType, VariantMap& eventData);
};
