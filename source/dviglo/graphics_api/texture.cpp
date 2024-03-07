// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include "../graphics/graphics.h"
#include "../graphics/material.h"
#include "graphics_impl.h"
#include "../io/file_system.h"
#include "../io/log.h"
#include "../resource/resource_cache.h"
#include "../resource/xml_file.h"

#include "../common/debug_new.h"

namespace dviglo
{

static const char* addressModeNames[] =
{
    "wrap",
    "mirror",
    "clamp",
    "border",
    nullptr
};

static const char* filterModeNames[] =
{
    "nearest",
    "bilinear",
    "trilinear",
    "anisotropic",
    "nearestanisotropic",
    "default",
    nullptr
};

Texture::Texture()
{
}

Texture::~Texture() = default;

void Texture::SetNumLevels(unsigned levels)
{
    if (usage_ > TEXTURE_RENDERTARGET)
        requestedLevels_ = 1;
    else
        requestedLevels_ = levels;
}

void Texture::SetFilterMode(TextureFilterMode mode)
{
    filterMode_ = mode;
    parametersDirty_ = true;
}

void Texture::SetAddressMode(TextureCoordinate coord, TextureAddressMode mode)
{
    addressModes_[coord] = mode;
    parametersDirty_ = true;
}

void Texture::SetAnisotropy(unsigned level)
{
    anisotropy_ = level;
    parametersDirty_ = true;
}

void Texture::SetShadowCompare(bool enable)
{
    shadowCompare_ = enable;
    parametersDirty_ = true;
}

void Texture::SetBorderColor(const Color& color)
{
    borderColor_ = color;
    parametersDirty_ = true;
}

void Texture::SetBackupTexture(Texture* texture)
{
    backupTexture_ = texture;
}

void Texture::SetMipsToSkip(MaterialQuality quality, int toSkip)
{
    if (quality >= QUALITY_LOW && quality < MAX_TEXTURE_QUALITY_LEVELS)
    {
        mipsToSkip_[quality] = (unsigned)toSkip;

        // Make sure a higher quality level does not actually skip more mips
        for (int i = 1; i < MAX_TEXTURE_QUALITY_LEVELS; ++i)
        {
            if (mipsToSkip_[i] > mipsToSkip_[i - 1])
                mipsToSkip_[i] = mipsToSkip_[i - 1];
        }
    }
}

int Texture::GetMipsToSkip(MaterialQuality quality) const
{
    return (quality >= QUALITY_LOW && quality < MAX_TEXTURE_QUALITY_LEVELS) ? mipsToSkip_[quality] : 0;
}

int Texture::GetLevelWidth(unsigned level) const
{
    if (level > levels_)
        return 0;
    return Max(width_ >> level, 1);
}

int Texture::GetLevelHeight(unsigned level) const
{
    if (level > levels_)
        return 0;
    return Max(height_ >> level, 1);
}

int Texture::GetLevelDepth(unsigned level) const
{
    if (level > levels_)
        return 0;
    return Max(depth_ >> level, 1);
}

unsigned Texture::GetDataSize(int width, int height) const
{
    if (IsCompressed())
        return GetRowDataSize(width) * ((height + 3) >> 2u);
    else
        return GetRowDataSize(width) * height;
}

unsigned Texture::GetDataSize(int width, int height, int depth) const
{
    return depth * GetDataSize(width, height);
}

unsigned Texture::GetComponents() const
{
    if (!width_ || IsCompressed())
        return 0;
    else
        return GetRowDataSize(width_) / width_;
}

void Texture::SetParameters(XmlFile* file)
{
    if (!file)
        return;

    XmlElement rootElem = file->GetRoot();
    SetParameters(rootElem);
}

void Texture::SetParameters(const XmlElement& element)
{
    load_metadata_from_xml(element);
    for (XmlElement paramElem = element.GetChild(); paramElem; paramElem = paramElem.GetNext())
    {
        String name = paramElem.GetName();

        if (name == "address")
        {
            String coord = paramElem.GetAttributeLower("coord");
            if (coord.Length() >= 1)
            {
                auto coordIndex = (TextureCoordinate)(coord[0] - 'u');
                String mode = paramElem.GetAttributeLower("mode");
                SetAddressMode(coordIndex, (TextureAddressMode)GetStringListIndex(mode.c_str(), addressModeNames, ADDRESS_WRAP));
            }
        }

        if (name == "border")
            SetBorderColor(paramElem.GetColor("color"));

        if (name == "filter")
        {
            String mode = paramElem.GetAttributeLower("mode");
            SetFilterMode((TextureFilterMode)GetStringListIndex(mode.c_str(), filterModeNames, FILTER_DEFAULT));
            if (paramElem.HasAttribute("anisotropy"))
                SetAnisotropy(paramElem.GetU32("anisotropy"));
        }

        if (name == "mipmap")
            SetNumLevels(paramElem.GetBool("enable") ? 0 : 1);

        if (name == "quality")
        {
            if (paramElem.HasAttribute("low"))
                SetMipsToSkip(QUALITY_LOW, paramElem.GetI32("low"));
            if (paramElem.HasAttribute("med"))
                SetMipsToSkip(QUALITY_MEDIUM, paramElem.GetI32("med"));
            if (paramElem.HasAttribute("medium"))
                SetMipsToSkip(QUALITY_MEDIUM, paramElem.GetI32("medium"));
            if (paramElem.HasAttribute("high"))
                SetMipsToSkip(QUALITY_HIGH, paramElem.GetI32("high"));
        }

        if (name == "srgb")
            SetSRGB(paramElem.GetBool("enable"));
    }
}

void Texture::SetParametersDirty()
{
    parametersDirty_ = true;
}

void Texture::SetLevelsDirty()
{
    if (usage_ == TEXTURE_RENDERTARGET && levels_ > 1)
        levelsDirty_ = true;
}

unsigned Texture::CheckMaxLevels(int width, int height, unsigned requestedLevels)
{
    unsigned maxLevels = 1;
    while (width > 1 || height > 1)
    {
        ++maxLevels;
        width = width > 1 ? (width >> 1u) : 1;
        height = height > 1 ? (height >> 1u) : 1;
    }

    if (!requestedLevels || maxLevels < requestedLevels)
        return maxLevels;
    else
        return requestedLevels;
}

unsigned Texture::CheckMaxLevels(int width, int height, int depth, unsigned requestedLevels)
{
    unsigned maxLevels = 1;
    while (width > 1 || height > 1 || depth > 1)
    {
        ++maxLevels;
        width = width > 1 ? (width >> 1u) : 1;
        height = height > 1 ? (height >> 1u) : 1;
        depth = depth > 1 ? (depth >> 1u) : 1;
    }

    if (!requestedLevels || maxLevels < requestedLevels)
        return maxLevels;
    else
        return requestedLevels;
}

void Texture::CheckTextureBudget(StringHash type)
{
    unsigned long long textureBudget = DV_RES_CACHE->GetMemoryBudget(type);
    unsigned long long textureUse = DV_RES_CACHE->GetMemoryUse(type);
    if (!textureBudget)
        return;

    // If textures are over the budget, they likely can not be freed directly as materials still refer to them.
    // Therefore free unused materials first
    if (textureUse > textureBudget)
        DV_RES_CACHE->release_resources(Material::GetTypeStatic());
}

void Texture::SetSRGB(bool enable)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetSRGB_OGL(enable);
#endif
}

void Texture::UpdateParameters()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return UpdateParameters_OGL();
#endif
}

bool Texture::GetParametersDirty() const
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetParametersDirty_OGL();
#endif

    return {}; // Prevent warning
}

bool Texture::IsCompressed() const
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return IsCompressed_OGL();
#endif

    return {}; // Prevent warning
}

unsigned Texture::GetRowDataSize(int width) const
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return GetRowDataSize_OGL(width);
#endif

    return {}; // Prevent warning
}

void Texture::regenerate_levels()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return RegenerateLevels_OGL();
#endif
}

}
