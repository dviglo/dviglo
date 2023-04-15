// Copyright (c) 2022-2023 the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#pragma once

#include <memory>

#include "game_object.h"

#include "ai_controller.h"

class Ninja : public GameObject
{
    DV_OBJECT(Ninja);

public:
    static void register_object();

    Controls controls;
    Controls prevControls;
    std::unique_ptr<AIController> controller;
    bool okToJump;
    bool smoke;
    float inAirTime;
    float onGroundTime;
    float throwTime;
    float deathTime;
    float deathDir;
    float dirChangeTime;
    float aimX;
    float aimY;

    Ninja();
    void DelayedStart() override;
    void SetControls(const Controls& newControls);
    Quaternion GetAim();
    void FixedUpdate(float timeStep) override;
    void DeathUpdate(float timeStep);
    bool Heal(i32 amount) override;
};
