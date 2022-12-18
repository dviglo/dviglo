// Copyright (c) 2008-2022 the Urho3D project
// Copyright (c) 2022-2022 the Dviglo project
// License: MIT

#include "../precompiled.h"

#include "../input/controls.h"

namespace Urho3D
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
