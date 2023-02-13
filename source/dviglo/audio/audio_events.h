// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

namespace dviglo
{

/// Sound playback finished. Sent through the SoundSource's Node.
URHO3D_EVENT(E_SOUNDFINISHED, SoundFinished)
{
    URHO3D_PARAM(P_NODE, Node);                     // Node pointer
    URHO3D_PARAM(P_SOUNDSOURCE, SoundSource);       // SoundSource pointer
    URHO3D_PARAM(P_SOUND, Sound);                   // Sound pointer
}

}
