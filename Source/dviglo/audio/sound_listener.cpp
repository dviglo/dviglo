// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../precompiled.h"

#include "../audio/sound_listener.h"
#include "../core/context.h"

namespace Urho3D
{

extern const char* AUDIO_CATEGORY;

SoundListener::SoundListener(Context* context) :
    Component(context)
{
}

SoundListener::~SoundListener() = default;

void SoundListener::RegisterObject(Context* context)
{
    context->RegisterFactory<SoundListener>(AUDIO_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, true, AM_DEFAULT);
}

}
