// Copyright (c) the Dviglo project
// License: MIT

// Функции, которые можно использовать до инициализации любых подсистем

#pragma once

#include "../containers/str.h"

namespace dviglo
{

DV_API bool dir_exists(const String& path);

/// Версия функции, которая не пишет с лог
DV_API bool create_dir_silent(const String& path);

/// Аналог SDL_GetPrefPath(), который не требует инициализации SDL
/// <https://github.com/libsdl-org/SDL/issues/2587>.
/// org может быть "".
/// В конце пути добавляет '/'.
/// В случае неудачи возвращает ""
DV_API String get_pref_path(const String& org, const String& app);

} // namespace dviglo
