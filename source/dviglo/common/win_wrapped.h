// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#ifdef _WIN32

// В файле rpcndr.h определён тип byte, который конфликтует с byte движка
#define byte BYTE

#include <windows.h>

#undef byte

#endif // def _WIN32

// Чтобы узнать, какой файл подключает windows.h без обёртки, можно воспользоваться опцией VS:
// ПКМ по проекту -> Properties -> Configuration Properties -> C/C++ -> Advanced -> Show Includes
