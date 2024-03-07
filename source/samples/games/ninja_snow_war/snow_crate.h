// Copyright (c) the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#pragma once

#include "game_object.h"

class SnowCrate : public GameObject
{
    DV_OBJECT(SnowCrate);

public:
    static void register_object();

    SnowCrate();

    void Start() override;
    void FixedUpdate(float timeStep) override;
};
