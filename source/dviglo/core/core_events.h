// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

namespace dviglo
{

/// Frame begin event.
DV_EVENT(E_BEGINFRAME, BeginFrame)
{
    DV_PARAM(P_FRAMENUMBER, FrameNumber);      // i32
    DV_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Application-wide logic update event.
DV_EVENT(E_UPDATE, Update)
{
    DV_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Application-wide logic post-update event.
DV_EVENT(E_POSTUPDATE, PostUpdate)
{
    DV_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Render update event.
DV_EVENT(E_RENDERUPDATE, RenderUpdate)
{
    DV_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Post-render update event.
DV_EVENT(E_POSTRENDERUPDATE, PostRenderUpdate)
{
    DV_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Frame end event.
DV_EVENT(E_ENDFRAME, EndFrame)
{
}

}
