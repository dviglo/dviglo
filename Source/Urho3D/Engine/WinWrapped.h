// Copyright (c) 2008-2022 the Urho3D project
// Copyright (c) 2022-2022 the Dviglo project
// License: MIT

#pragma once

#ifdef _WIN32

// Avoid using byte in rpcndr.h
#define byte BYTE
#include <windows.h>
#undef byte

#endif // def _WIN32
