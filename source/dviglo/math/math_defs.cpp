// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../precompiled.h"

#include "../math/math_defs.h"

#include "../debug_new.h"

namespace Urho3D
{

void SinCos(float angle, float& sin, float& cos)
{
    float angleRadians = angle * M_DEGTORAD;

#ifdef _MSC_VER
    sin = sinf(angleRadians);
    cos = cosf(angleRadians);
#else // Linux или MinGW
    sincosf(angleRadians, &sin, &cos);
#endif
}

}
