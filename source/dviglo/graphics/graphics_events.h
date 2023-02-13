// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

namespace dviglo
{

/// New screen mode set.
DV_EVENT(E_SCREENMODE, ScreenMode)
{
    DV_PARAM(P_WIDTH, Width);                  // int
    DV_PARAM(P_HEIGHT, Height);                // int
    DV_PARAM(P_FULLSCREEN, Fullscreen);        // bool
    DV_PARAM(P_BORDERLESS, Borderless);        // bool
    DV_PARAM(P_RESIZABLE, Resizable);          // bool
    DV_PARAM(P_HIGHDPI, HighDPI);              // bool
    DV_PARAM(P_MONITOR, Monitor);              // int
    DV_PARAM(P_REFRESHRATE, RefreshRate);      // int
}

/// Window position changed.
DV_EVENT(E_WINDOWPOS, WindowPos)
{
    DV_PARAM(P_X, X);                          // int
    DV_PARAM(P_Y, Y);                          // int
}

/// Request for queuing rendersurfaces either in manual or always-update mode.
DV_EVENT(E_RENDERSURFACEUPDATE, RenderSurfaceUpdate)
{
}

/// Frame rendering started.
DV_EVENT(E_BEGINRENDERING, BeginRendering)
{
}

/// Frame rendering ended.
DV_EVENT(E_ENDRENDERING, EndRendering)
{
}

/// Update of a view started.
DV_EVENT(E_BEGINVIEWUPDATE, BeginViewUpdate)
{
    DV_PARAM(P_VIEW, View);                    // View pointer
    DV_PARAM(P_TEXTURE, Texture);              // Texture pointer
    DV_PARAM(P_SURFACE, Surface);              // RenderSurface pointer
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_CAMERA, Camera);                // Camera pointer
}

/// Update of a view ended.
DV_EVENT(E_ENDVIEWUPDATE, EndViewUpdate)
{
    DV_PARAM(P_VIEW, View);                    // View pointer
    DV_PARAM(P_TEXTURE, Texture);              // Texture pointer
    DV_PARAM(P_SURFACE, Surface);              // RenderSurface pointer
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_CAMERA, Camera);                // Camera pointer
}

/// Render of a view started.
DV_EVENT(E_BEGINVIEWRENDER, BeginViewRender)
{
    DV_PARAM(P_VIEW, View);                    // View pointer
    DV_PARAM(P_TEXTURE, Texture);              // Texture pointer
    DV_PARAM(P_SURFACE, Surface);              // RenderSurface pointer
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_CAMERA, Camera);                // Camera pointer
}

/// A view has allocated its screen buffers for rendering. They can be accessed now with View::FindNamedTexture().
DV_EVENT(E_VIEWBUFFERSREADY, ViewBuffersReady)
{
    DV_PARAM(P_VIEW, View);                    // View pointer
    DV_PARAM(P_TEXTURE, Texture);              // Texture pointer
    DV_PARAM(P_SURFACE, Surface);              // RenderSurface pointer
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_CAMERA, Camera);                // Camera pointer
}

/// A view has set global shader parameters for a new combination of vertex/pixel shaders. Custom global parameters can now be set.
DV_EVENT(E_VIEWGLOBALSHADERPARAMETERS, ViewGlobalShaderParameters)
{
    DV_PARAM(P_VIEW, View);                    // View pointer
    DV_PARAM(P_TEXTURE, Texture);              // Texture pointer
    DV_PARAM(P_SURFACE, Surface);              // RenderSurface pointer
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_CAMERA, Camera);                // Camera pointer
}

/// Render of a view ended. Its screen buffers are still accessible if needed.
DV_EVENT(E_ENDVIEWRENDER, EndViewRender)
{
    DV_PARAM(P_VIEW, View);                    // View pointer
    DV_PARAM(P_TEXTURE, Texture);              // Texture pointer
    DV_PARAM(P_SURFACE, Surface);              // RenderSurface pointer
    DV_PARAM(P_SCENE, Scene);                  // Scene pointer
    DV_PARAM(P_CAMERA, Camera);                // Camera pointer
}

/// Render of all views is finished for the frame.
DV_EVENT(E_ENDALLVIEWSRENDER, EndAllViewsRender)
{
}

/// A render path event has occurred.
DV_EVENT(E_RENDERPATHEVENT, RenderPathEvent)
{
    DV_PARAM(P_NAME, Name);                    // String
}

/// Graphics context has been lost. Some or all (depending on the API) GPU objects have lost their contents.
DV_EVENT(E_DEVICELOST, DeviceLost)
{
}

/// Graphics context has been recreated after being lost. GPU objects in the "data lost" state can be restored now.
DV_EVENT(E_DEVICERESET, DeviceReset)
{
}

}
