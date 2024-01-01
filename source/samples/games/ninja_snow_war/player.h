// Copyright (c) 2022-2024 the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#pragma once

#include <dviglo/dviglo_all.h>

using namespace dviglo;

struct Player
{
    i32 score;
    String name;
    NodeId nodeID;
    WeakPtr<Connection> connection;
    Controls lastControls;

    Player();
};

struct HiscoreEntry
{
    i32 score;
    String name;
};
