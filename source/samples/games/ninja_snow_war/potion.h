// Copyright (c) 2022-2023 the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#pragma once

#include "game_object.h"

namespace dviglo
{

class Potion : public GameObject
{
    DV_OBJECT(Potion, GameObject);

private:
    i32 healAmount;

public:
    static void RegisterObject(Context* context);

    Potion(Context* context);
    void Start() override;
    void ObjectCollision(GameObject& otherObject, VariantMap& eventData) override;
};

} // namespace dviglo
