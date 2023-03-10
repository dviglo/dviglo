// Copyright (c) 2022-2023 the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#pragma once

#include "game_object.h"

namespace dviglo
{

class LightFlash : public GameObject
{
    DV_OBJECT(LightFlash, GameObject);

public:
    static void RegisterObject();

    LightFlash();
    void FixedUpdate(float timeStep) override;
};

} // namespace dviglo
