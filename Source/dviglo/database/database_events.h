// Copyright (c) 2008-2022 the Urho3D project
// Copyright (c) 2022-2022 the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"
#include "../database/db_connection.h"

namespace Urho3D
{

/// %Database cursor. Event handler could set P_FILTER to true to filter out a row from resultset and P_ABORT to true to stop further cursor events.
URHO3D_EVENT(E_DBCURSOR, DbCursor)
{
    URHO3D_PARAM(P_DBCONNECTION, DbConnection);    // DbConnection pointer
    URHO3D_PARAM(P_RESULTIMPL, ResultImpl);        // Underlying result object pointer (cannot be used in scripting)
    URHO3D_PARAM(P_SQL, SQL);                      // String
    URHO3D_PARAM(P_NUMCOLS, NumCols);              // unsigned
    URHO3D_PARAM(P_COLVALUES, ColValues);          // VariantVector
    URHO3D_PARAM(P_COLHEADERS, ColHeaders);        // StringVector
    URHO3D_PARAM(P_FILTER, Filter);                // bool [in]
    URHO3D_PARAM(P_ABORT, Abort);                  // bool [in]
}

}
