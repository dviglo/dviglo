// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"


namespace dviglo
{

/// Async system command execution finished.
DV_EVENT(E_ASYNCEXECFINISHED, AsyncExecFinished)
{
    DV_PARAM(P_REQUESTID, RequestID);          // unsigned
    DV_PARAM(P_EXITCODE, ExitCode);            // int
}

} // namespace dviglo
