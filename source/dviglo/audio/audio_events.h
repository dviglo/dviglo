// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

namespace dviglo
{

/// Sound playback finished. Sent through the SoundSource's Node.
DV_EVENT(E_SOUNDFINISHED, SoundFinished)
{
    DV_PARAM(P_NODE, Node);                     // Node pointer
    DV_PARAM(P_SOUNDSOURCE, SoundSource);       // SoundSource pointer
    DV_PARAM(P_SOUND, Sound);                   // Sound pointer
}

}
