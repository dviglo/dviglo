// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#include "sdl_helper.h"

#include "../io/log.h"

#include <SDL3/SDL.h>

#include "../common/debug_new.h"

namespace dviglo
{

SdlHelper::SdlHelper()
{
    instance_ = this;

    DV_LOGDEBUG("SdlHelper constructed");
}

SdlHelper::~SdlHelper()
{
    DV_LOGDEBUG("Quitting SDL");
    SDL_Quit();
    instance_ = nullptr;
    DV_LOGDEBUG("SdlHelper destructed");
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
