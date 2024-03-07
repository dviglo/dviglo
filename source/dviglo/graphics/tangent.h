// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "../common/config.h"

namespace dviglo
{

/// Generate tangents to indexed geometry.
DV_API void GenerateTangents
    (void* vertexData, unsigned vertexSize, const void* indexData, unsigned indexSize, unsigned indexStart, unsigned indexCount,
        unsigned normalOffset, unsigned texCoordOffset, unsigned tangentOffset);

}
