// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

namespace dviglo
{

/// Log message event.
DV_EVENT(E_LOGMESSAGE, LogMessage)
{
    DV_PARAM(P_MESSAGE, Message);              // String
    DV_PARAM(P_LEVEL, Level);                  // int
}

/// Async system command execution finished.
DV_EVENT(E_ASYNCEXECFINISHED, AsyncExecFinished)
{
    DV_PARAM(P_REQUESTID, RequestID);          // unsigned
    DV_PARAM(P_EXITCODE, ExitCode);            // int
}

}
