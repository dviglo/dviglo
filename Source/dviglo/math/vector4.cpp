// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../precompiled.h"

#include "../math/vector4.h"

#include <cstdio>

#include "../debug_new.h"

namespace Urho3D
{

const Vector4 Vector4::ZERO;
const Vector4 Vector4::ONE(1.0f, 1.0f, 1.0f, 1.0f);

String Vector4::ToString() const
{
    char tempBuffer[CONVERSION_BUFFER_LENGTH];
    sprintf(tempBuffer, "%g %g %g %g", x_, y_, z_, w_);
    return String(tempBuffer);
}

}
