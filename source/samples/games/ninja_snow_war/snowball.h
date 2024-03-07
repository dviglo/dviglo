// Copyright (c) the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#pragma once

#include "game_object.h"

class Snowball : public GameObject
{
    DV_OBJECT(Snowball);

private:
    int hitDamage;

public:
    static void register_object();

    Snowball();

    void Start() override;
    void FixedUpdate(float timeStep) override;
    void WorldCollision(VariantMap& eventData) override;
    void ObjectCollision(GameObject& otherObject, VariantMap& eventData) override;
};
