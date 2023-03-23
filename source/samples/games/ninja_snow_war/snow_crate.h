// Copyright (c) 2022-2023 the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#pragma once

#include "game_object.h"

namespace dviglo
{

class SnowCrate : public GameObject
{
    DV_OBJECT(SnowCrate, GameObject);

public:
    static void register_object();

    SnowCrate();

    void Start() override;
    void FixedUpdate(float timeStep) override;
};

} // namespace dviglo
