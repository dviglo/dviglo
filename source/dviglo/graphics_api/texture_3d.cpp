// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../core/profiler.h"
#include "../graphics/graphics.h"
#include "../graphics/graphics_events.h"
#include "../graphics/renderer.h"
#include "graphics_impl.h"
#include "texture_3d.h"
#include "../io/file_system.h"
#include "../io/log.h"
#include "../resource/resource_cache.h"
#include "../resource/xml_file.h"

#include "../common/debug_new.h"

namespace dviglo
{

Texture3D::Texture3D()
{
#ifdef DV_OPENGL
#ifndef DV_GLES2
    if (GParams::get_gapi() == GAPI_OPENGL)
        target_ = GL_TEXTURE_3D;
#endif
#endif
}

Texture3D::~Texture3D()
{
    Release();
}

void Texture3D::register_object()
{
    DV_CONTEXT.RegisterFactory<Texture3D>();
}

bool Texture3D::begin_load(Deserializer& source)
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

    String texPath, texName, texExt;
    split_path(GetName(), texPath, texName, texExt);

    DV_RES_CACHE.reset_dependencies(this);

    loadParameters_ = new XmlFile();
    if (!loadParameters_->Load(source))
    {
        loadParameters_.Reset();
        return false;
    }

    XmlElement textureElem = loadParameters_->GetRoot();
    XmlElement volumeElem = textureElem.GetChild("volume");
    XmlElement colorlutElem = textureElem.GetChild("colorlut");

    if (volumeElem)
    {
        String name = volumeElem.GetAttribute("name");

        String volumeTexPath, volumeTexName, volumeTexExt;
        split_path(name, volumeTexPath, volumeTexName, volumeTexExt);
        // If path is empty, add the XML file path
        if (volumeTexPath.Empty())
            name = texPath + name;

        loadImage_ = DV_RES_CACHE.GetTempResource<Image>(name);
        // Precalculate mip levels if async loading
        if (loadImage_ && GetAsyncLoadState() == ASYNC_LOADING)
            loadImage_->PrecalculateLevels();
        DV_RES_CACHE.store_resource_dependency(this, name);
        return true;
    }
    else if (colorlutElem)
    {
        String name = colorlutElem.GetAttribute("name");

        String colorlutTexPath, colorlutTexName, colorlutTexExt;
        split_path(name, colorlutTexPath, colorlutTexName, colorlutTexExt);
        // If path is empty, add the XML file path
        if (colorlutTexPath.Empty())
            name = texPath + name;

        SharedPtr<File> file = DV_RES_CACHE.GetFile(name);
        loadImage_ = new Image();
        if (!loadImage_->LoadColorLUT(*(file.Get())))
        {
            loadParameters_.Reset();
            loadImage_.Reset();
            return false;
        }
        // Precalculate mip levels if async loading
        if (loadImage_ && GetAsyncLoadState() == ASYNC_LOADING)
            loadImage_->PrecalculateLevels();
        DV_RES_CACHE.store_resource_dependency(this, name);
        return true;
    }

    DV_LOGERROR("Texture3D XML data for " + GetName() + " did not contain either volume or colorlut element");
    return false;
}


bool Texture3D::end_load()
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

bool Texture3D::SetSize(int width, int height, int depth, unsigned format, TextureUsage usage)
{
    if (width <= 0 || height <= 0 || depth <= 0)
    {
        DV_LOGERROR("Zero or negative 3D texture dimensions");
        return false;
    }
    if (usage >= TEXTURE_RENDERTARGET)
    {
        DV_LOGERROR("Rendertarget or depth-stencil usage not supported for 3D textures");
        return false;
    }

    usage_ = usage;

    width_ = width;
    height_ = height;
    depth_ = depth;
    format_ = format;

    return Create();
}

void Texture3D::OnDeviceLost()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return OnDeviceLost_OGL();
#endif
}

void Texture3D::OnDeviceReset()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return OnDeviceReset_OGL();
#endif
}

void Texture3D::Release()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Release_OGL();
#endif
}

bool Texture3D::SetData(unsigned level, int x, int y, int z, int width, int height, int depth, const void* data)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetData_OGL(level, x, y, z, width, height, depth, data);
#endif

    return {}; // Prevent warning
}

bool Texture3D::SetData(Image* image, bool useAlpha)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetData_OGL(image, useAlpha);
#endif

    return {}; // Prevent warning
}

bool Texture3D::GetData(unsigned level, void* dest) const
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetData_OGL(level, dest);
#endif

    return {}; // Prevent warning
}

bool Texture3D::Create()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Create_OGL();
#endif

    return {}; // Prevent warning
}

}
