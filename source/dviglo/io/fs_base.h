// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

// Функции, которые можно использовать до инициализации любых подсистем

#pragma once

#include "../containers/str.h"

namespace dviglo
{

/// В Windows заменяет '/' на '\\'
inline String to_native(const String& path)
{
#ifdef _WIN32
    return path.Replaced('/', '\\');
#else
    return path;
#endif
}

} // namespace dviglo
