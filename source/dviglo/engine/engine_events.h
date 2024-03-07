// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

namespace dviglo
{

/// A command has been entered on the console.
DV_EVENT(E_CONSOLECOMMAND, ConsoleCommand)
{
    DV_PARAM(P_COMMAND, Command);              // String
    DV_PARAM(P_ID, Id);                        // String
}

}
