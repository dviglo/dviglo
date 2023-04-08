// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "sdl_helper.h"

#include "../io/log.h"

#include <SDL3/SDL.h>

#include "../common/debug_new.h"

namespace dviglo
{

#ifndef NDEBUG
// Проверяем, что не происходит обращения к синглтону после вызова деструктора
static bool sdl_helper_destructed = false;
#endif

// Определение должно быть в cpp-файле, иначе будут проблемы в shared-версии движка в MinGW.
// Когда функция в h-файле, в exe и в dll создаются свои экземпляры объекта с разными адресами.
// https://stackoverflow.com/questions/71830151/why-singleton-in-headers-not-work-for-windows-mingw
SdlHelper& SdlHelper::get_instance()
{
    assert(!sdl_helper_destructed);
    static SdlHelper instance;
    return instance;
}

SdlHelper::SdlHelper()
{
    // Гарантируем, что синглтон будет создан после лога
    Log::get_instance();

    DV_LOGDEBUG("Singleton SdlHelper constructed");
}

void SdlHelper::manual_destruct()
{
    DV_LOGDEBUG("Quitting SDL");
    SDL_Quit();
    DV_LOGDEBUG("Singleton SdlHelper destructed");

#ifndef NDEBUG
    sdl_helper_destructed = true;
#endif
}

SdlHelper::~SdlHelper()
{
    // Вызов SDL_Quit() тут вызывает крэш в dll версии движка в Windows
    /*
    DV_LOGDEBUG("Quitting SDL");
    SDL_Quit();
    DV_LOGDEBUG("Singleton SdlHelper destructed");

#ifndef NDEBUG
    sdl_helper_destructed = true;
#endif
    */
}

bool SdlHelper::require(u32 sdl_subsystem)
{
    if (!sdl_inited_)
    {
        DV_LOGDEBUG("Initializing SDL");

        // Сперва инициализируем без подсистем, так как при инициализации подсистем могут возникнуть
        // допустимые ошибки (например может не инициализироваться аудио в Linux)
        if (SDL_Init(0) != 0)
        {
            DV_LOGERRORF("Failed to initialize SDL: %s", SDL_GetError());
            return false;
        }

        sdl_inited_ = true;
    }

    if (SDL_InitSubSystem(sdl_subsystem) != 0)
    {
        DV_LOGERRORF("Failed to initialize SDL subsystem: %s", SDL_GetError());
        return false;
    }

    return true;
}

} // namespace dviglo
