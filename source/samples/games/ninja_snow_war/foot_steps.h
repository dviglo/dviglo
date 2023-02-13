// Copyright (c) 2022-2023 the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#pragma once

#include <dviglo/dviglo_all.h>

namespace dviglo
{

class FootSteps : public LogicComponent
{
    URHO3D_OBJECT(FootSteps, LogicComponent);

public:
    static void RegisterObject(Context* context);

    FootSteps(Context* context);

    void Start() override;
    void HandleAnimationTrigger(StringHash eventType, VariantMap& eventData);
};

} // namespace dviglo
