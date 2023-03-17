// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../common/config.h"
#include "../common/primitive_types.h"

namespace dviglo
{

class DV_API SdlHelper
{
public:
    static SdlHelper& get_instance();

    // Запрещаем копирование
    SdlHelper(const SdlHelper&) = delete;
    SdlHelper& operator =(const SdlHelper&) = delete;

    /// Желательно запрашивать по одной подсистеме за раз, так как при ошибке инициализации одной подсистемы
    /// другие тоже могут не проинициализироваться
    bool require(u32 sdl_subsystem);

    /// В Windows, когда движок скомпилирован как dll, при вызове SDL_Quit() в деструкторе синглтона
    /// происходит крэш. Даже если поместить SDL_Quit() в atexit() в библиотеке, то проблема остается. В описании
    /// SDL_Quit() об этом и написано. См также https://stackoverflow.com/questions/19402417/when-should-atexit-be-used
    /// Поэтому завершаем SDL вручную. При этом нарушается порядок уничтожения синглтонов (SDL уничтожается первым),
    /// но вроде всё работает.
    static void manual_destruct();

private:
    bool sdl_inited_ = false;

    SdlHelper();
    ~SdlHelper();
};

#define DV_SDL_HELPER (dviglo::SdlHelper::get_instance())

} // namespace dviglo
