// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../common/config.h"

#include "../common/primitive_types.h"

namespace dviglo
{

/// Set the random seed. The default seed is 1.
DV_API void set_random_seed(unsigned seed);
/// Return the current random seed.
DV_API unsigned GetRandomSeed();
/// Return a random number between 0-32767. Should operate similarly to MSVC rand().
DV_API int Rand();
/// Return a standard normal distributed number.
DV_API float RandStandardNormal();

}
