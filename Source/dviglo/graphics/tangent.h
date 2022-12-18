// Copyright (c) 2008-2022 the Urho3D project
// Copyright (c) 2022-2022 the Dviglo project
// License: MIT

#pragma once

#ifdef URHO3D_IS_BUILDING
#include "dviglo.h"
#else
#include <dviglo/dviglo.h>
#endif

namespace Urho3D
{

/// Generate tangents to indexed geometry.
URHO3D_API void GenerateTangents
    (void* vertexData, unsigned vertexSize, const void* indexData, unsigned indexSize, unsigned indexStart, unsigned indexCount,
        unsigned normalOffset, unsigned texCoordOffset, unsigned tangentOffset);

}
