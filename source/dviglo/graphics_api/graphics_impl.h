// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

// Note: GraphicsImpl class is purposefully API-specific. It should not be used by Urho3D client applications,
// unless required for e.g. integration of 3rd party libraries that interface directly with the graphics device.

#ifdef URHO3D_OPENGL
#include "opengl/ogl_graphics_impl.h"
#endif

#ifdef URHO3D_D3D11
#include "direct3d11/d3d11_graphics_impl.h"
#endif
