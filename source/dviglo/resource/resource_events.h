// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

namespace dviglo
{

/// Resource reloading started.
DV_EVENT(E_RELOADSTARTED, ReloadStarted)
{
}

/// Resource reloading finished successfully.
DV_EVENT(E_RELOADFINISHED, ReloadFinished)
{
}

/// Resource reloading failed.
DV_EVENT(E_RELOADFAILED, ReloadFailed)
{
}

/// Tracked file changed in the resource directories.
DV_EVENT(E_FILECHANGED, FileChanged)
{
    DV_PARAM(P_FILENAME, FileName);                    // String
    DV_PARAM(P_RESOURCENAME, ResourceName);            // String
}

/// Resource loading failed.
DV_EVENT(E_LOADFAILED, LoadFailed)
{
    DV_PARAM(P_RESOURCENAME, ResourceName);            // String
}

/// Resource not found.
DV_EVENT(E_RESOURCENOTFOUND, ResourceNotFound)
{
    DV_PARAM(P_RESOURCENAME, ResourceName);            // String
}

/// Unknown resource type.
DV_EVENT(E_UNKNOWNRESOURCETYPE, UnknownResourceType)
{
    DV_PARAM(P_RESOURCETYPE, ResourceType);            // StringHash
}

/// Resource background loading finished.
DV_EVENT(E_RESOURCEBACKGROUNDLOADED, ResourceBackgroundLoaded)
{
    DV_PARAM(P_RESOURCENAME, ResourceName);            // String
    DV_PARAM(P_SUCCESS, Success);                      // bool
    DV_PARAM(P_RESOURCE, Resource);                    // Resource pointer
}

/// Language changed.
DV_EVENT(E_CHANGELANGUAGE, ChangeLanguage)
{
}

}
