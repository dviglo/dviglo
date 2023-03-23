// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "sound_listener.h"
#include "../core/context.h"

namespace dviglo
{

extern const char* AUDIO_CATEGORY;

SoundListener::SoundListener()
{
}

SoundListener::~SoundListener() = default;

void SoundListener::register_object()
{
    DV_CONTEXT.RegisterFactory<SoundListener>(AUDIO_CATEGORY);

    DV_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, true, AM_DEFAULT);
}

}
