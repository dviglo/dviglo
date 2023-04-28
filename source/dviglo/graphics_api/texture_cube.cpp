// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../core/profiler.h"
#include "../graphics/graphics.h"
#include "../graphics/graphics_events.h"
#include "../graphics/renderer.h"
#include "graphics_impl.h"
#include "texture_cube.h"
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

static const char* cubeMapLayoutNames[] = {
    "horizontal",
    "horizontalnvidia",
    "horizontalcross",
    "verticalcross",
    "blender",
    nullptr
};

static SharedPtr<Image> GetTileImage(Image* src, int tileX, int tileY, int tileWidth, int tileHeight)
{
    return SharedPtr<Image>(
        src->GetSubimage(IntRect(tileX * tileWidth, tileY * tileHeight, (tileX + 1) * tileWidth, (tileY + 1) * tileHeight)));
}

TextureCube::TextureCube()
{
#ifdef DV_OPENGL
    if (GParams::get_gapi() == GAPI_OPENGL)
        target_ = GL_TEXTURE_CUBE_MAP;
#endif

    // Default to clamp mode addressing
    addressModes_[COORD_U] = ADDRESS_CLAMP;
    addressModes_[COORD_V] = ADDRESS_CLAMP;
    addressModes_[COORD_W] = ADDRESS_CLAMP;
}

TextureCube::~TextureCube()
{
    Release();
}

void TextureCube::register_object()
{
    DV_CONTEXT.RegisterFactory<TextureCube>();
}

bool TextureCube::begin_load(Deserializer& source)
{
    ResourceCache* cache = DV_RES_CACHE;

    // In headless mode, do not actually load the texture, just return success
    if (GParams::is_headless())
        return true;

    // If device is lost, retry later
    if (DV_GRAPHICS->IsDeviceLost())
    {
        DV_LOGWARNING("Texture load while device is lost");
        dataPending_ = true;
        return true;
    }

    cache->reset_dependencies(this);

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
    XmlElement imageElem = textureElem.GetChild("image");
    // Single image and multiple faces with layout
    if (imageElem)
    {
        String name = imageElem.GetAttribute("name");
        // If path is empty, add the XML file path
        if (GetPath(name).Empty())
            name = texPath + name;

        SharedPtr<Image> image = cache->GetTempResource<Image>(name);
        if (!image)
            return false;

        int faceWidth, faceHeight;
        loadImages_.Resize(MAX_CUBEMAP_FACES);

        if (image->IsCubemap())
        {
            loadImages_[FACE_POSITIVE_X] = image;
            loadImages_[FACE_NEGATIVE_X] = loadImages_[FACE_POSITIVE_X]->GetNextSibling();
            loadImages_[FACE_POSITIVE_Y] = loadImages_[FACE_NEGATIVE_X]->GetNextSibling();
            loadImages_[FACE_NEGATIVE_Y] = loadImages_[FACE_POSITIVE_Y]->GetNextSibling();
            loadImages_[FACE_POSITIVE_Z] = loadImages_[FACE_NEGATIVE_Y]->GetNextSibling();
            loadImages_[FACE_NEGATIVE_Z] = loadImages_[FACE_POSITIVE_Z]->GetNextSibling();
        }
        else
        {

            CubeMapLayout layout =
                (CubeMapLayout)GetStringListIndex(imageElem.GetAttribute("layout").c_str(), cubeMapLayoutNames, CML_HORIZONTAL);

            switch (layout)
            {
            case CML_HORIZONTAL:
                faceWidth = image->GetWidth() / MAX_CUBEMAP_FACES;
                faceHeight = image->GetHeight();
                loadImages_[FACE_POSITIVE_Z] = GetTileImage(image, 0, 0, faceWidth, faceHeight);
                loadImages_[FACE_POSITIVE_X] = GetTileImage(image, 1, 0, faceWidth, faceHeight);
                loadImages_[FACE_NEGATIVE_Z] = GetTileImage(image, 2, 0, faceWidth, faceHeight);
                loadImages_[FACE_NEGATIVE_X] = GetTileImage(image, 3, 0, faceWidth, faceHeight);
                loadImages_[FACE_POSITIVE_Y] = GetTileImage(image, 4, 0, faceWidth, faceHeight);
                loadImages_[FACE_NEGATIVE_Y] = GetTileImage(image, 5, 0, faceWidth, faceHeight);
                break;

            case CML_HORIZONTALNVIDIA:
                faceWidth = image->GetWidth() / MAX_CUBEMAP_FACES;
                faceHeight = image->GetHeight();
                for (unsigned i = 0; i < MAX_CUBEMAP_FACES; ++i)
                    loadImages_[i] = GetTileImage(image, i, 0, faceWidth, faceHeight);
                break;

            case CML_HORIZONTALCROSS:
                faceWidth = image->GetWidth() / 4;
                faceHeight = image->GetHeight() / 3;
                loadImages_[FACE_POSITIVE_Y] = GetTileImage(image, 1, 0, faceWidth, faceHeight);
                loadImages_[FACE_NEGATIVE_X] = GetTileImage(image, 0, 1, faceWidth, faceHeight);
                loadImages_[FACE_POSITIVE_Z] = GetTileImage(image, 1, 1, faceWidth, faceHeight);
                loadImages_[FACE_POSITIVE_X] = GetTileImage(image, 2, 1, faceWidth, faceHeight);
                loadImages_[FACE_NEGATIVE_Z] = GetTileImage(image, 3, 1, faceWidth, faceHeight);
                loadImages_[FACE_NEGATIVE_Y] = GetTileImage(image, 1, 2, faceWidth, faceHeight);
                break;

            case CML_VERTICALCROSS:
                faceWidth = image->GetWidth() / 3;
                faceHeight = image->GetHeight() / 4;
                loadImages_[FACE_POSITIVE_Y] = GetTileImage(image, 1, 0, faceWidth, faceHeight);
                loadImages_[FACE_NEGATIVE_X] = GetTileImage(image, 0, 1, faceWidth, faceHeight);
                loadImages_[FACE_POSITIVE_Z] = GetTileImage(image, 1, 1, faceWidth, faceHeight);
                loadImages_[FACE_POSITIVE_X] = GetTileImage(image, 2, 1, faceWidth, faceHeight);
                loadImages_[FACE_NEGATIVE_Y] = GetTileImage(image, 1, 2, faceWidth, faceHeight);
                loadImages_[FACE_NEGATIVE_Z] = GetTileImage(image, 1, 3, faceWidth, faceHeight);
                if (loadImages_[FACE_NEGATIVE_Z])
                {
                    loadImages_[FACE_NEGATIVE_Z]->FlipVertical();
                    loadImages_[FACE_NEGATIVE_Z]->FlipHorizontal();
                }
                break;

            case CML_BLENDER:
                faceWidth = image->GetWidth() / 3;
                faceHeight = image->GetHeight() / 2;
                loadImages_[FACE_NEGATIVE_X] = GetTileImage(image, 0, 0, faceWidth, faceHeight);
                loadImages_[FACE_NEGATIVE_Z] = GetTileImage(image, 1, 0, faceWidth, faceHeight);
                loadImages_[FACE_POSITIVE_X] = GetTileImage(image, 2, 0, faceWidth, faceHeight);
                loadImages_[FACE_NEGATIVE_Y] = GetTileImage(image, 0, 1, faceWidth, faceHeight);
                loadImages_[FACE_POSITIVE_Y] = GetTileImage(image, 1, 1, faceWidth, faceHeight);
                loadImages_[FACE_POSITIVE_Z] = GetTileImage(image, 2, 1, faceWidth, faceHeight);
                break;
            }
        }
    }
    // Face per image
    else
    {
        XmlElement faceElem = textureElem.GetChild("face");
        while (faceElem)
        {
            String name = faceElem.GetAttribute("name");

            // If path is empty, add the XML file path
            if (GetPath(name).Empty())
                name = texPath + name;

            loadImages_.Push(cache->GetTempResource<Image>(name));
            cache->store_resource_dependency(this, name);

            faceElem = faceElem.GetNext("face");
        }
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

bool TextureCube::end_load()
{
    // In headless mode, do not actually load the texture, just return success
    if (GParams::is_headless() || DV_GRAPHICS->IsDeviceLost())
        return true;

    // If over the texture budget, see if materials can be freed to allow textures to be freed
    CheckTextureBudget(GetTypeStatic());

    SetParameters(loadParameters_);

    for (unsigned i = 0; i < loadImages_.Size() && i < MAX_CUBEMAP_FACES; ++i)
        SetData((CubeMapFace)i, loadImages_[i]);

    loadImages_.Clear();
    loadParameters_.Reset();

    return true;
}

bool TextureCube::SetSize(int size, unsigned format, TextureUsage usage, int multiSample)
{
    if (size <= 0)
    {
        DV_LOGERROR("Zero or negative cube texture size");
        return false;
    }
    if (usage == TEXTURE_DEPTHSTENCIL)
    {
        DV_LOGERROR("Depth-stencil usage not supported for cube textures");
        return false;
    }

    multiSample = Clamp(multiSample, 1, 16);
    if (multiSample > 1 && usage < TEXTURE_RENDERTARGET)
    {
        DV_LOGERROR("Multisampling is only supported for rendertarget cube textures");
        return false;
    }

    // Delete the old rendersurfaces if any
    for (unsigned i = 0; i < MAX_CUBEMAP_FACES; ++i)
    {
        renderSurfaces_[i].Reset();
        faceMemoryUse_[i] = 0;
    }

    usage_ = usage;

    if (usage == TEXTURE_RENDERTARGET)
    {
        for (unsigned i = 0; i < MAX_CUBEMAP_FACES; ++i)
        {
            renderSurfaces_[i] = new RenderSurface(this);
#ifdef DV_OPENGL
            if (GParams::get_gapi() == GAPI_OPENGL)
                renderSurfaces_[i]->target_ = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
#endif
        }

        // Nearest filtering by default
        filterMode_ = FILTER_NEAREST;
    }

    if (usage == TEXTURE_RENDERTARGET)
        subscribe_to_event(E_RENDERSURFACEUPDATE, DV_HANDLER(TextureCube, HandleRenderSurfaceUpdate));
    else
        unsubscribe_from_event(E_RENDERSURFACEUPDATE);

    width_ = size;
    height_ = size;
    depth_ = 1;
    format_ = format;
    multiSample_ = multiSample;
    autoResolve_ = multiSample > 1;

    return Create();
}

SharedPtr<Image> TextureCube::GetImage(CubeMapFace face) const
{
    if (format_ != Graphics::GetRGBAFormat() && format_ != Graphics::GetRGBFormat())
    {
        DV_LOGERROR("Unsupported texture format, can not convert to Image");
        return SharedPtr<Image>();
    }

    auto* rawImage = new Image();
    if (format_ == Graphics::GetRGBAFormat())
        rawImage->SetSize(width_, height_, 4);
    else if (format_ == Graphics::GetRGBFormat())
        rawImage->SetSize(width_, height_, 3);
    else
        assert(false);

    GetData(face, 0, rawImage->GetData());
    return SharedPtr<Image>(rawImage);
}

void TextureCube::HandleRenderSurfaceUpdate(StringHash eventType, VariantMap& eventData)
{
    for (auto& renderSurface : renderSurfaces_)
    {
        if (renderSurface && (renderSurface->GetUpdateMode() == SURFACE_UPDATEALWAYS || renderSurface->IsUpdateQueued()))
        {
            if (!GParams::is_headless())
                DV_RENDERER->QueueRenderSurface(renderSurface);
            renderSurface->ResetUpdateQueued();
        }
    }
}

void TextureCube::OnDeviceLost()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return OnDeviceLost_OGL();
#endif
}

void TextureCube::OnDeviceReset()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return OnDeviceReset_OGL();
#endif
}

void TextureCube::Release()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Release_OGL();
#endif
}

bool TextureCube::SetData(CubeMapFace face, unsigned level, int x, int y, int width, int height, const void* data)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetData_OGL(face, level, x, y, width, height, data);
#endif

    return {}; // Prevent warning
}

bool TextureCube::SetData(CubeMapFace face, Deserializer& source)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetData_OGL(face, source);
#endif

    return {}; // Prevent warning
}

bool TextureCube::SetData(CubeMapFace face, Image* image, bool useAlpha)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetData_OGL(face, image, useAlpha);
#endif

    return {}; // Prevent warning
}

bool TextureCube::GetData(CubeMapFace face, unsigned level, void* dest) const
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetData_OGL(face, level, dest);
#endif

    return {}; // Prevent warning
}

bool TextureCube::Create()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Create_OGL();
#endif

    return {}; // Prevent warning
}

}
