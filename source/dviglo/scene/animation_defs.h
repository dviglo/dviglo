// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

/// \file

#pragma once

namespace dviglo
{

/// Animation wrap mode.
enum WrapMode
{
    /// Loop mode.
    WM_LOOP = 0,
    /// Play once, when animation finished it will be removed.
    WM_ONCE,
    /// Clamp mode.
    WM_CLAMP,
};

}

