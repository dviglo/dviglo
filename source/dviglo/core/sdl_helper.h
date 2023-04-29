// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../common/config.h"
#include "../common/primitive_types.h"

namespace dviglo
{

class DV_API SdlHelper
{
    /// Только Application может создать и уничтожить объект
    friend class Application;

private:
    /// Инициализируется в конструкторе
    inline static SdlHelper* instance_ = nullptr;

public:
    static SdlHelper* instance() { return instance_; }

    // Запрещаем копирование
    SdlHelper(const SdlHelper&) = delete;
    SdlHelper& operator =(const SdlHelper&) = delete;

    /// Желательно запрашивать по одной подсистеме за раз, так как при ошибке инициализации одной подсистемы
    /// другие тоже могут не проинициализироваться
    bool require(u32 sdl_subsystem);

private:
    bool sdl_inited_ = false;

    SdlHelper();
    ~SdlHelper();
};

#define DV_SDL_HELPER (dviglo::SdlHelper::instance())

} // namespace dviglo
