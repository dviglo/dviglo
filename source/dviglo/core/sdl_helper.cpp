// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "sdl_helper.h"

#include "../io/log.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gesture.h>

#include "../common/debug_new.h"

namespace dviglo
{

// Определение должно быть в cpp-файле, иначе будут проблемы в shared-версии движка в MinGW.
// Когда функция в h-файле, в exe и в dll создаются свои экземпляры объекта с разными адресами.
// https://stackoverflow.com/questions/71830151/why-singleton-in-headers-not-work-for-windows-mingw
SdlHelper& SdlHelper::get_instance()
{
    static SdlHelper instance;
    return instance;
}

SdlHelper::SdlHelper()
{
    // Гарантируем, что синглтон будет создан после лога
    Log::get_instance();
}

SdlHelper::~SdlHelper()
{
    DV_LOGDEBUG("Quitting SDL");
    Gesture_Quit();
    SDL_Quit();
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

        // Начиная с SDL 3 Gesture больше не часть библиотеки
        Gesture_Init(); // Всегда возвращает 0

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
