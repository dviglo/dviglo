// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "../scene/component.h"

namespace dviglo
{

/// %Sound listener component.
class DV_API SoundListener : public Component
{
    DV_OBJECT(SoundListener);

public:
    /// Construct.
    explicit SoundListener();
    /// Destruct.
    ~SoundListener() override;
    /// Register object factory.
    static void register_object();
};

}
