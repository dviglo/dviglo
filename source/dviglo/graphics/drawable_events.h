// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

namespace dviglo
{

/// AnimatedModel bone hierarchy created.
DV_EVENT(E_BONEHIERARCHYCREATED, BoneHierarchyCreated)
{
    DV_PARAM(P_NODE, Node);                    // Node pointer
}

/// AnimatedModel animation trigger.
DV_EVENT(E_ANIMATIONTRIGGER, AnimationTrigger)
{
    DV_PARAM(P_NODE, Node);                    // Node pointer
    DV_PARAM(P_ANIMATION, Animation);          // Animation pointer
    DV_PARAM(P_NAME, Name);                    // String
    DV_PARAM(P_TIME, Time);                    // Float
    DV_PARAM(P_DATA, Data);                    // User-defined data type
}

/// AnimatedModel animation finished or looped.
DV_EVENT(E_ANIMATIONFINISHED, AnimationFinished)
{
    DV_PARAM(P_NODE, Node);                    // Node pointer
    DV_PARAM(P_ANIMATION, Animation);          // Animation pointer
    DV_PARAM(P_NAME, Name);                    // String
    DV_PARAM(P_LOOPED, Looped);                // Bool
}

/// Particle effect finished.
DV_EVENT(E_PARTICLEEFFECTFINISHED, ParticleEffectFinished)
{
    DV_PARAM(P_NODE, Node);                    // Node pointer
    DV_PARAM(P_EFFECT, Effect);                // ParticleEffect pointer
}

/// Terrain geometry created.
DV_EVENT(E_TERRAINCREATED, TerrainCreated)
{
    DV_PARAM(P_NODE, Node);                    // Node pointer
}

}
