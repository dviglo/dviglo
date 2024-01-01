// Copyright (c) 2022-2024 the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#include "free_type_lib_helper.h"

#include "../io/log.h"


namespace dviglo
{

FreeTypeLibHelper::FreeTypeLibHelper()
{
    FT_Error error = FT_Init_FreeType(&library_);

    if (error)
        DV_LOGERROR("Could not initialize FreeType library");

    instance_ = this;

    DV_LOGDEBUG("FreeTypeLibHelper constructed");
}

FreeTypeLibHelper::~FreeTypeLibHelper()
{
    FT_Done_FreeType(library_);
    instance_ = nullptr;
    DV_LOGDEBUG("FreeTypeLibrary destructed");
}

} // namespace dviglo
