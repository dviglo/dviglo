// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

namespace dviglo
{

/// Emitting ParticleEmitter2D particles stopped.
DV_EVENT(E_PARTICLESEND, ParticlesEnd)
{
    DV_PARAM(P_NODE, Node);                    // Node pointer
    DV_PARAM(P_EFFECT, Effect);                // ParticleEffect2D pointer
}

/// All ParticleEmitter2D particles have been removed.
DV_EVENT(E_PARTICLESDURATION, ParticlesDuration)
{
    DV_PARAM(P_NODE, Node);                    // Node pointer
    DV_PARAM(P_EFFECT, Effect);                // ParticleEffect2D pointer
}

}
