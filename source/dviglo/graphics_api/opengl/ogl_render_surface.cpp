// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../../graphics/camera.h"
#include "../../graphics/graphics.h"
#include "../../graphics/renderer.h"
#include "../graphics_impl.h"
#include "../render_surface.h"
#include "../texture.h"

#include "../../common/debug_new.h"

#ifdef GL_ES_VERSION_2_0
#define GL_RENDERBUFFER_EXT GL_RENDERBUFFER
#define glGenRenderbuffersEXT glGenRenderbuffers
#define glBindRenderbufferEXT glBindRenderbuffer
#define glRenderbufferStorageEXT glRenderbufferStorage
#define glDeleteRenderbuffersEXT glDeleteRenderbuffers
#define glRenderbufferStorageMultisampleEXT glRenderbufferStorageMultisample
#endif

namespace dviglo
{

void RenderSurface::Constructor_OGL(Texture* parentTexture)
{
    parentTexture_ = parentTexture;
    target_ = GL_TEXTURE_2D;
    renderBuffer_ = 0;
}

bool RenderSurface::CreateRenderBuffer_OGL(unsigned width, unsigned height, unsigned format, int multiSample)
{
    if (GParams::is_headless())
        return false;

    Release_OGL();

#ifndef GL_ES_VERSION_2_0
    if (Graphics::GetGL3Support())
    {
        glGenRenderbuffers(1, &renderBuffer_);
        glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer_);
        if (multiSample > 1)
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, multiSample, format, width, height);
        else
            glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
    else
#endif
    {
        glGenRenderbuffersEXT(1, &renderBuffer_);
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderBuffer_);
#ifndef DV_GLES2
        if (multiSample > 1)
            glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, multiSample, format, width, height);
        else
#endif
            glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, format, width, height);
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
    }

    return true;
}

void RenderSurface::OnDeviceLost_OGL()
{
    if (GParams::is_headless())
        return;

    Graphics& graphics = DV_GRAPHICS;

    for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
    {
        if (graphics.GetRenderTarget(i) == this)
            graphics.ResetRenderTarget(i);
    }

    if (graphics.GetDepthStencil() == this)
        graphics.ResetDepthStencil();

    // Clean up also from non-active FBOs
    graphics.CleanupRenderSurface_OGL(this);

    if (renderBuffer_ && !graphics.IsDeviceLost())
        glDeleteRenderbuffersEXT(1, &renderBuffer_);

    renderBuffer_ = 0;
}

void RenderSurface::Release_OGL()
{
    if (GParams::is_headless() || Graphics::is_destructed())
        return;

    Graphics& graphics = DV_GRAPHICS;

    if (!graphics.IsDeviceLost())
    {
        for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
        {
            if (graphics.GetRenderTarget(i) == this)
                graphics.ResetRenderTarget(i);
        }

        if (graphics.GetDepthStencil() == this)
            graphics.ResetDepthStencil();

        // Clean up also from non-active FBOs
        graphics.CleanupRenderSurface_OGL(this);

        if (renderBuffer_)
            glDeleteRenderbuffersEXT(1, &renderBuffer_);
    }

    renderBuffer_ = 0;
}

}
