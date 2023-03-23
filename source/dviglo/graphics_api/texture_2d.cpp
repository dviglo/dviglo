// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../core/profiler.h"
#include "../graphics/graphics.h"
#include "../graphics/graphics_events.h"
#include "../graphics/renderer.h"
#include "graphics_impl.h"
#include "texture_2d.h"
#include "../io/file_system.h"
#include "../io/log.h"
#include "../resource/resource_cache.h"
#include "../resource/xml_file.h"

#include "../common/debug_new.h"

namespace dviglo
{

Texture2D::Texture2D()
{
#ifdef DV_OPENGL
    if (GParams::get_gapi() == GAPI_OPENGL)
        target_ = GL_TEXTURE_2D;
#endif
}

Texture2D::~Texture2D()
{
    Release();
}

void Texture2D::register_object()
{
    DV_CONTEXT.RegisterFactory<Texture2D>();
}

bool Texture2D::BeginLoad(Deserializer& source)
{
    // In headless mode, do not actually load the texture, just return success
    if (GParams::is_headless())
        return true;

    // If device is lost, retry later
    if (DV_GRAPHICS.IsDeviceLost())
    {
        DV_LOGWARNING("Texture load while device is lost");
        dataPending_ = true;
        return true;
    }

    // Load the image data for EndLoad()
    loadImage_ = new Image();
    if (!loadImage_->Load(source))
    {
        loadImage_.Reset();
        return false;
    }

    // Precalculate mip levels if async loading
    if (GetAsyncLoadState() == ASYNC_LOADING)
        loadImage_->PrecalculateLevels();

    // Load the optional parameters file
    String xmlName = ReplaceExtension(GetName(), ".xml");
    loadParameters_ = DV_RES_CACHE.GetTempResource<XmlFile>(xmlName, false);

    return true;
}

bool Texture2D::EndLoad()
{
    // In headless mode, do not actually load the texture, just return success
    if (GParams::is_headless() || DV_GRAPHICS.IsDeviceLost())
        return true;

    // If over the texture budget, see if materials can be freed to allow textures to be freed
    CheckTextureBudget(GetTypeStatic());

    SetParameters(loadParameters_);
    bool success = SetData(loadImage_);

    loadImage_.Reset();
    loadParameters_.Reset();

    return success;
}

bool Texture2D::SetSize(int width, int height, unsigned format, TextureUsage usage, int multiSample, bool autoResolve)
{
    if (width <= 0 || height <= 0)
    {
        DV_LOGERROR("Zero or negative texture dimensions");
        return false;
    }

    multiSample = Clamp(multiSample, 1, 16);
    if (multiSample == 1)
        autoResolve = false;
    else if (multiSample > 1 && usage < TEXTURE_RENDERTARGET)
    {
        DV_LOGERROR("Multisampling is only supported for rendertarget or depth-stencil textures");
        return false;
    }

    // Disable mipmaps if multisample & custom resolve
    if (multiSample > 1 && autoResolve == false)
        requestedLevels_ = 1;

    // Delete the old rendersurface if any
    renderSurface_.Reset();

    usage_ = usage;

    if (usage >= TEXTURE_RENDERTARGET)
    {
        renderSurface_ = new RenderSurface(this);

        // Clamp mode addressing by default and nearest filtering
        addressModes_[COORD_U] = ADDRESS_CLAMP;
        addressModes_[COORD_V] = ADDRESS_CLAMP;
        filterMode_ = FILTER_NEAREST;
    }

    if (usage == TEXTURE_RENDERTARGET)
        SubscribeToEvent(E_RENDERSURFACEUPDATE, DV_HANDLER(Texture2D, HandleRenderSurfaceUpdate));
    else
        UnsubscribeFromEvent(E_RENDERSURFACEUPDATE);

    width_ = width;
    height_ = height;
    format_ = format;
    depth_ = 1;
    multiSample_ = multiSample;
    autoResolve_ = autoResolve;

    return Create();
}

bool Texture2D::GetImage(Image& image) const
{
    if (format_ != Graphics::GetRGBAFormat() && format_ != Graphics::GetRGBFormat())
    {
        DV_LOGERROR("Unsupported texture format, can not convert to Image");
        return false;
    }

    image.SetSize(width_, height_, GetComponents());
    GetData(0, image.GetData());
    return true;
}

SharedPtr<Image> Texture2D::GetImage() const
{
    auto rawImage = MakeShared<Image>();
    if (!GetImage(*rawImage))
        return nullptr;
    return rawImage;
}

void Texture2D::HandleRenderSurfaceUpdate(StringHash eventType, VariantMap& eventData)
{
    if (renderSurface_ && (renderSurface_->GetUpdateMode() == SURFACE_UPDATEALWAYS || renderSurface_->IsUpdateQueued()))
    {
        if (!GParams::is_headless())
            DV_RENDERER.QueueRenderSurface(renderSurface_);
        renderSurface_->ResetUpdateQueued();
    }
}

void Texture2D::OnDeviceLost()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return OnDeviceLost_OGL();
#endif
}

void Texture2D::OnDeviceReset()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return OnDeviceReset_OGL();
#endif
}

void Texture2D::Release()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Release_OGL();
#endif
}

bool Texture2D::SetData(unsigned level, int x, int y, int width, int height, const void* data)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetData_OGL(level, x, y, width, height, data);
#endif

    return {}; // Prevent warning
}

bool Texture2D::SetData(Image* image, bool useAlpha)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetData_OGL(image, useAlpha);
#endif

    return {}; // Prevent warning
}

bool Texture2D::GetData(unsigned level, void* dest) const
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetData_OGL(level, dest);
#endif

    return {}; // Prevent warning
}

bool Texture2D::Create()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Create_OGL();
#endif

    return {}; // Prevent warning
}

}
