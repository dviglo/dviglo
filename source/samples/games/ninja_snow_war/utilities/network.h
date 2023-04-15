// Copyright (c) 2022-2023 the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#pragma once

#include <dviglo/containers/str.h>

using namespace dviglo;

inline bool runServer = false;
inline bool runClient = false;
inline String serverAddress;
inline u16 serverPort = 1234;
inline String userName;
inline bool nobgm = false; // Без фоновой музыки

void ParseNetworkArguments();
