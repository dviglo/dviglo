// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../common/config.h"

namespace dviglo
{

#if defined(_MSC_VER) && defined(DV_MINIDUMPS)
/// Write a minidump. Needs to be called from within a structured exception handler.
DV_API int WriteMiniDump(const char* applicationName, void* exceptionPointers);
#endif

}

