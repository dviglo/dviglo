// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#ifdef URHO3D_DATABASE_ODBC
#include "odbc/odbc_connection.h"
#elif defined(URHO3D_DATABASE_SQLITE)
#include "sqlite/sqlite_connection.h"
#else
#error "Database subsystem not enabled"
#endif
