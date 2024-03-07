// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include "vector4.h"

#include <cstdio>

#include "../common/debug_new.h"

namespace dviglo
{

const Vector4 Vector4::ZERO;
const Vector4 Vector4::ONE(1.0f, 1.0f, 1.0f, 1.0f);

String Vector4::ToString() const
{
    char tempBuffer[CONVERSION_BUFFER_LENGTH];
    sprintf(tempBuffer, "%g %g %g %g", x, y, z, w);
    return String(tempBuffer);
}

}
