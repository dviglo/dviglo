// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#include "vector_base.h"

#include "../common/debug_new.h"

namespace dviglo
{

u8* VectorBase::AllocateBuffer(i32 size)
{
    return new u8[size];
}

}
