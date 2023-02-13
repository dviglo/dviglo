// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../precompiled.h"

#include "../container/vector_base.h"

#include "../debug_new.h"

namespace dviglo
{

u8* VectorBase::AllocateBuffer(i32 size)
{
    return new u8[size];
}

}
