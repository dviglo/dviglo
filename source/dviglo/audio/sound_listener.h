// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../scene/component.h"

namespace Urho3D
{

/// %Sound listener component.
class URHO3D_API SoundListener : public Component
{
    URHO3D_OBJECT(SoundListener, Component);

public:
    /// Construct.
    explicit SoundListener(Context* context);
    /// Destruct.
    ~SoundListener() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);
};

}
