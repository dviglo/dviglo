// Copyright (c) 2008-2022 the Urho3D project
// Copyright (c) 2022-2022 the Dviglo project
// License: MIT

#pragma once

#ifdef URHO3D_DATABASE_ODBC
#include "odbc/odbc_result.h"
#elif defined(URHO3D_DATABASE_SQLITE)
#include "sqlite/sqlite_result.h"
#else
#error "Database subsystem not enabled"
#endif
