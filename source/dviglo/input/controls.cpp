// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#include "controls.h"

namespace dviglo
{

Controls::Controls() :
    buttons_(0),
    yaw_(0.f),
    pitch_(0.f)
{
}

Controls::~Controls() = default;

void Controls::Reset()
{
    buttons_ = 0;
    yaw_ = 0.0f;
    pitch_ = 0.0f;
    extraData_.Clear();
}

}
