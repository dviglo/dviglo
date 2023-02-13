// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../scene/component.h"

namespace dviglo
{

/// %Sound listener component.
class DV_API SoundListener : public Component
{
    DV_OBJECT(SoundListener, Component);

public:
    /// Construct.
    explicit SoundListener(Context* context);
    /// Destruct.
    ~SoundListener() override;
    /// Register object factory.
    static void RegisterObject(Context* context);
};

}
