// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../core/profiler.h"
#include "../graphics/graphics.h"
#include "../graphics/graphics_events.h"
#include "../graphics/renderer.h"
#include "graphics_impl.h"
#include "texture_2d_array.h"
#include "../io/file_system.h"
#include "../io/log.h"
#include "../resource/resource_cache.h"
#include "../resource/xml_file.h"

#include "../common/debug_new.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

namespace dviglo
{

Texture2DArray::Texture2DArray()
{
#ifdef DV_OPENGL
#ifndef DV_GLES2
    if (GParams::get_gapi() == GAPI_OPENGL)
        target_ = GL_TEXTURE_2D_ARRAY;
#endif
#endif
}

Texture2DArray::~Texture2DArray()
{
    Release();
}

void Texture2DArray::register_object()
{
    DV_CONTEXT.RegisterFactory<Texture2DArray>();
}

bool Texture2DArray::BeginLoad(Deserializer& source)
{
    ResourceCache& cache = DV_RES_CACHE;

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

    cache.reset_dependencies(this);

    String texPath, texName, texExt;
    split_path(GetName(), texPath, texName, texExt);

    loadParameters_ = (new XmlFile());
    if (!loadParameters_->Load(source))
    {
        loadParameters_.Reset();
        return false;
    }

    loadImages_.Clear();

    XmlElement textureElem = loadParameters_->GetRoot();
    XmlElement layerElem = textureElem.GetChild("layer");
    while (layerElem)
    {
        String name = layerElem.GetAttribute("name");

        // If path is empty, add the XML file path
        if (GetPath(name).Empty())
            name = texPath + name;

        loadImages_.Push(cache.GetTempResource<Image>(name));
        cache.store_resource_dependency(this, name);

        layerElem = layerElem.GetNext("layer");
    }

    // Precalculate mip levels if async loading
    if (GetAsyncLoadState() == ASYNC_LOADING)
    {
        for (unsigned i = 0; i < loadImages_.Size(); ++i)
        {
            if (loadImages_[i])
                loadImages_[i]->PrecalculateLevels();
        }
    }

    return true;
}

bool Texture2DArray::EndLoad()
{
    // In headless mode, do not actually load the texture, just return success
    if (GParams::is_headless() || DV_GRAPHICS.IsDeviceLost())
        return true;

    // If over the texture budget, see if materials can be freed to allow textures to be freed
    CheckTextureBudget(GetTypeStatic());

    SetParameters(loadParameters_);
    SetLayers(loadImages_.Size());

    for (unsigned i = 0; i < loadImages_.Size(); ++i)
        SetData(i, loadImages_[i]);

    loadImages_.Clear();
    loadParameters_.Reset();

    return true;
}

void Texture2DArray::SetLayers(unsigned layers)
{
    Release();

    layers_ = layers;
}

bool Texture2DArray::SetSize(unsigned layers, int width, int height, unsigned format, TextureUsage usage)
{
    if (width <= 0 || height <= 0)
    {
        DV_LOGERROR("Zero or negative texture array size");
        return false;
    }
    if (usage == TEXTURE_DEPTHSTENCIL)
    {
        DV_LOGERROR("Depth-stencil usage not supported for texture arrays");
        return false;
    }

    // Delete the old rendersurface if any
    renderSurface_.Reset();

    usage_ = usage;

    if (usage == TEXTURE_RENDERTARGET)
    {
        renderSurface_ = new RenderSurface(this);

        // Nearest filtering by default
        filterMode_ = FILTER_NEAREST;
    }

    if (usage == TEXTURE_RENDERTARGET)
        subscribe_to_event(E_RENDERSURFACEUPDATE, DV_HANDLER(Texture2DArray, HandleRenderSurfaceUpdate));
    else
        unsubscribe_from_event(E_RENDERSURFACEUPDATE);

    width_ = width;
    height_ = height;
    format_ = format;
    depth_ = 1;
    if (layers)
        layers_ = layers;

    layerMemoryUse_.Resize(layers_);
    for (unsigned i = 0; i < layers_; ++i)
        layerMemoryUse_[i] = 0;

    return Create();
}

void Texture2DArray::HandleRenderSurfaceUpdate(StringHash eventType, VariantMap& eventData)
{
    if (renderSurface_ && (renderSurface_->GetUpdateMode() == SURFACE_UPDATEALWAYS || renderSurface_->IsUpdateQueued()))
    {
        if (!GParams::is_headless())
            DV_RENDERER.QueueRenderSurface(renderSurface_);
        renderSurface_->ResetUpdateQueued();
    }
}

void Texture2DArray::OnDeviceLost()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return OnDeviceLost_OGL();
#endif
}

void Texture2DArray::OnDeviceReset()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return OnDeviceReset_OGL();
#endif
}

void Texture2DArray::Release()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Release_OGL();
#endif
}

bool Texture2DArray::SetData(unsigned layer, unsigned level, int x, int y, int width, int height, const void* data)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetData_OGL(layer, level, x, y, width, height, data);
#endif

    return {}; // Prevent warning
}

bool Texture2DArray::SetData(unsigned layer, Deserializer& source)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetData_OGL(layer, source);
#endif

    return {}; // Prevent warning
}

bool Texture2DArray::SetData(unsigned layer, Image* image, bool useAlpha)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetData_OGL(layer, image, useAlpha);
#endif

    return {}; // Prevent warning
}

bool Texture2DArray::GetData(unsigned layer, unsigned level, void* dest) const
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetData_OGL(layer, level, dest);
#endif

    return {}; // Prevent warning
}

bool Texture2DArray::Create()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Create_OGL();
#endif

    return {}; // Prevent warning
}

}
