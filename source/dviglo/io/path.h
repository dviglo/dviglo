// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

// Функции для работы с файловыми путями.
// Можно использовать до инициализации любых подсистем

#pragma once

#include "../containers/str.h"

namespace dviglo
{

/// Заменяет '\\' на '/'
inline String to_internal(const String& path)
{
    return path.Replaced('\\', '/');
}

/// В Windows заменяет '/' на '\\'
inline String to_native(const String& path)
{
#ifdef _WIN32
    return path.Replaced('/', '\\');
#else
    return path;
#endif
}

/// Удаляет один '/' в конце строки, если он есть
inline String trim_end_slash(const String& path)
{
    String ret = path;

    if (!ret.Empty() && ret.Back() == '/')
        ret.Resize(ret.Length() - 1);

    return ret;
}

} // namespace dviglo
