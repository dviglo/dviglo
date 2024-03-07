// Copyright (c) the Dviglo project
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

#ifdef _WIN32
/// В Windows заменяет '/' на '\\' и преобразует в кодировку UTF-16.
/// В Linux используется кодировка UTF-8, поэтому преобразование не требуется
inline WString to_win_native(const String& path)
{
    return WString(path.Replaced('/', '\\'));
}
#endif

/// Удаляет один '/' в конце строки, если он есть
inline String trim_end_slash(const String& path)
{
    String ret = path;

    if (!ret.Empty() && ret.Back() == '/')
        ret.Resize(ret.Length() - 1);

    return ret;
}

/// Возвращает родительский путь (с / в конце) или пустую строку
inline String get_parent(const String& path)
{
    i32 pos = trim_end_slash(path).FindLast('/');

    if (pos != String::NPOS)
        return path.Substring(0, pos + 1);
    else
        return String();
}

} // namespace dviglo
