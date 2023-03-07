// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#ifdef DV_TRACY_PROFILING
#define TRACY_ENABLE 1
#include "tracy/Tracy.hpp"
#endif

namespace dviglo
{

#ifdef DV_TRACY_PROFILING // Use Tracy profiler
    /// Macro for scoped profiling with a name.
    #define DV_PROFILE(name) ZoneScopedN(#name)
#else // Profiling off
    #define DV_PROFILE(name)
#endif

#ifdef DV_TRACY_PROFILING // Use Tracy profiler
    /// Macro for scoped profiling with a name and color.
    #define DV_PROFILE_COLOR(name, color) ZoneScopedNC(#name, color)
    /// Macro for scoped profiling with a dynamic string name.
    #define DV_PROFILE_STR(nameStr, size) ZoneName(nameStr, size)
    /// Macro for marking a game frame.
    #define DV_PROFILE_FRAME() FrameMark
    /// Macro for recording name of current thread.
    #define DV_PROFILE_THREAD(name) tracy::SetThreadName(name)
    /// Macro for scoped profiling of a function.
    #define DV_PROFILE_FUNCTION() ZoneScopedN(__FUNCTION__)

    /// Color used for highlighting event.
    #define DV_PROFILE_EVENT_COLOR tracy::Color::OrangeRed
    /// Color used for highlighting resource.
    #define DV_PROFILE_RESOURCE_COLOR tracy::Color::MediumSeaGreen
#else // Tracy profiler off
    #define DV_PROFILE_COLOR(name, color)
    #define DV_PROFILE_STR(nameStr, size)
    #define DV_PROFILE_FRAME()
    #define DV_PROFILE_THREAD(name)
    #define DV_PROFILE_FUNCTION()

    #define DV_PROFILE_EVENT_COLOR
    #define DV_PROFILE_RESOURCE_COLOR
#endif

}
