// Copyright (c) the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#pragma once

#include "game_object.h"

class LightFlash : public GameObject
{
    DV_OBJECT(LightFlash);

public:
    static void register_object();

    LightFlash();
    void FixedUpdate(float timeStep) override;
};
