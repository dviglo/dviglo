// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../../core/context.h"
#include "../../core/profiler.h"
#include "../../graphics/graphics.h"
#include "../../graphics/graphics_events.h"
#include "../../graphics/renderer.h"
#include "../graphics_impl.h"
#include "../texture_3d.h"
#include "../../io/file_system.h"
#include "../../io/log.h"
#include "../../resource/resource_cache.h"
#include "../../resource/xml_file.h"

#include "../../common/debug_new.h"

namespace dviglo
{

void Texture3D::OnDeviceLost_OGL()
{
    if (gpu_object_name_ && !DV_GRAPHICS.IsDeviceLost())
        glDeleteTextures(1, &gpu_object_name_);

    GpuObject::OnDeviceLost();
}

void Texture3D::OnDeviceReset_OGL()
{
    if (!gpu_object_name_ || dataPending_)
    {
        // If has a resource file, reload through the resource cache. Otherwise just recreate.
        if (DV_RES_CACHE.Exists(GetName()))
            dataLost_ = !DV_RES_CACHE.reload_resource(this);

        if (!gpu_object_name_)
        {
            Create_OGL();
            dataLost_ = true;
        }
    }

    dataPending_ = false;
}

void Texture3D::Release_OGL()
{
    if (gpu_object_name_)
    {
        if (GParams::is_headless() || DV_GRAPHICS.IsDeviceLost())
            return;

        Graphics& graphics = DV_GRAPHICS;

        for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
        {
            if (graphics.GetTexture(i) == this)
                graphics.SetTexture(i, nullptr);
        }

        glDeleteTextures(1, &gpu_object_name_);
        gpu_object_name_ = 0;
    }
}

bool Texture3D::SetData_OGL(unsigned level, int x, int y, int z, int width, int height, int depth, const void* data)
{
    DV_PROFILE(SetTextureData);

    if (!gpu_object_name_ || GParams::is_headless())
    {
        DV_LOGERROR("No texture created, can not set data");
        return false;
    }

    if (!data)
    {
        DV_LOGERROR("Null source for setting data");
        return false;
    }

    if (level >= levels_)
    {
        DV_LOGERROR("Illegal mip level for setting data");
        return false;
    }

    Graphics& graphics = DV_GRAPHICS;

    if (graphics.IsDeviceLost())
    {
        DV_LOGWARNING("Texture data assignment while device is lost");
        dataPending_ = true;
        return true;
    }

    if (IsCompressed_OGL())
    {
        x &= ~3u;
        y &= ~3u;
    }

    int levelWidth = GetLevelWidth(level);
    int levelHeight = GetLevelHeight(level);
    int levelDepth = GetLevelDepth(level);
    if (x < 0 || x + width > levelWidth || y < 0 || y + height > levelHeight || z < 0 || z + depth > levelDepth || width <= 0 ||
        height <= 0 || depth <= 0)
    {
        DV_LOGERROR("Illegal dimensions for setting data");
        return false;
    }

    graphics.SetTextureForUpdate_OGL(this);

#ifndef DV_GLES2
    bool wholeLevel = x == 0 && y == 0 && z == 0 && width == levelWidth && height == levelHeight && depth == levelDepth;
    unsigned format = GetSRGB() ? GetSRGBFormat_OGL(format_) : format_;

    if (!IsCompressed_OGL())
    {
        if (wholeLevel)
            glTexImage3D(target_, level, format, width, height, depth, 0, GetExternalFormat_OGL(format_), GetDataType_OGL(format_), data);
        else
            glTexSubImage3D(target_, level, x, y, z, width, height, depth, GetExternalFormat_OGL(format_), GetDataType_OGL(format_), data);
    }
    else
    {
        if (wholeLevel)
            glCompressedTexImage3D(target_, level, format, width, height, depth, 0, GetDataSize(width, height, depth), data);
        else
            glCompressedTexSubImage3D(target_, level, x, y, z, width, height, depth, format, GetDataSize(width, height, depth),
                data);
    }
#endif

    graphics.SetTexture(0, nullptr);
    return true;
}

bool Texture3D::SetData_OGL(Image* image, bool useAlpha)
{
    if (!image)
    {
        DV_LOGERROR("Null image, can not set data");
        return false;
    }

    // Use a shared ptr for managing the temporary mip images created during this function
    SharedPtr<Image> mipImage;
    unsigned memoryUse = sizeof(Texture3D);
    MaterialQuality quality = QUALITY_HIGH;
    if (!GParams::is_headless())
        quality = DV_RENDERER->GetTextureQuality();

    if (!image->IsCompressed())
    {
        // Convert unsuitable formats to RGBA
        unsigned components = image->GetComponents();
        if ((components == 1 && !useAlpha) || components == 2)
        {
            mipImage = image->ConvertToRGBA(); image = mipImage;
            if (!image)
                return false;
            components = image->GetComponents();
        }

        unsigned char* levelData = image->GetData();
        int levelWidth = image->GetWidth();
        int levelHeight = image->GetHeight();
        int levelDepth = image->GetDepth();
        unsigned format = 0;

        // Discard unnecessary mip levels
        for (unsigned i = 0; i < mipsToSkip_[quality]; ++i)
        {
            mipImage = image->GetNextLevel(); image = mipImage;
            levelData = image->GetData();
            levelWidth = image->GetWidth();
            levelHeight = image->GetHeight();
            levelDepth = image->GetDepth();
        }

        switch (components)
        {
        case 1:
            format = useAlpha ? Graphics::GetAlphaFormat() : Graphics::GetLuminanceFormat();
            break;

        case 2:
            format = Graphics::GetLuminanceAlphaFormat();
            break;

        case 3:
            format = Graphics::GetRGBFormat();
            break;

        case 4:
            format = Graphics::GetRGBAFormat();
            break;

        default:
            assert(false);  // Should not reach here
            break;
        }

        // If image was previously compressed, reset number of requested levels to avoid error if level count is too high for new size
        if (IsCompressed_OGL() && requestedLevels_ > 1)
            requestedLevels_ = 0;
        SetSize(levelWidth, levelHeight, levelDepth, format);
        if (!gpu_object_name_)
            return false;

        for (unsigned i = 0; i < levels_; ++i)
        {
            SetData_OGL(i, 0, 0, 0, levelWidth, levelHeight, levelDepth, levelData);
            memoryUse += levelWidth * levelHeight * levelDepth * components;

            if (i < levels_ - 1)
            {
                mipImage = image->GetNextLevel(); image = mipImage;
                levelData = image->GetData();
                levelWidth = image->GetWidth();
                levelHeight = image->GetHeight();
                levelDepth = image->GetDepth();
            }
        }
    }
    else
    {
        int width = image->GetWidth();
        int height = image->GetHeight();
        int depth = image->GetDepth();
        unsigned levels = image->GetNumCompressedLevels();
        unsigned format = DV_GRAPHICS.GetFormat(image->GetCompressedFormat());
        bool needDecompress = false;

        if (!format)
        {
            format = Graphics::GetRGBAFormat();
            needDecompress = true;
        }

        unsigned mipsToSkip = mipsToSkip_[quality];
        if (mipsToSkip >= levels)
            mipsToSkip = levels - 1;
        while (mipsToSkip && (width / (1u << mipsToSkip) < 4 || height / (1u << mipsToSkip) < 4 || depth / (1u << mipsToSkip) < 4))
            --mipsToSkip;
        width /= (1u << mipsToSkip);
        height /= (1u << mipsToSkip);
        depth /= (1u << mipsToSkip);

        SetNumLevels(Max((levels - mipsToSkip), 1U));
        SetSize(width, height, depth, format);

        for (unsigned i = 0; i < levels_ && i < levels - mipsToSkip; ++i)
        {
            CompressedLevel level = image->GetCompressedLevel(i + mipsToSkip);
            if (!needDecompress)
            {
                SetData_OGL(i, 0, 0, 0, level.width_, level.height_, level.depth_, level.data_);
                memoryUse += level.depth_ * level.rows_ * level.rowSize_;
            }
            else
            {
                auto* rgbaData = new unsigned char[level.width_ * level.height_ * level.depth_ * 4];
                level.Decompress(rgbaData);
                SetData_OGL(i, 0, 0, 0, level.width_, level.height_, level.depth_, rgbaData);
                memoryUse += level.width_ * level.height_ * level.depth_ * 4;
                delete[] rgbaData;
            }
        }
    }

    SetMemoryUse(memoryUse);
    return true;
}

bool Texture3D::GetData_OGL(unsigned level, void* dest) const
{
#ifndef GL_ES_VERSION_2_0
    if (!gpu_object_name_ || GParams::is_headless())
    {
        DV_LOGERROR("No texture created, can not get data");
        return false;
    }

    if (!dest)
    {
        DV_LOGERROR("Null destination for getting data");
        return false;
    }

    if (level >= levels_)
    {
        DV_LOGERROR("Illegal mip level for getting data");
        return false;
    }

    Graphics& graphics = DV_GRAPHICS;

    if (graphics.IsDeviceLost())
    {
        DV_LOGWARNING("Getting texture data while device is lost");
        return false;
    }

    graphics.SetTextureForUpdate_OGL(const_cast<Texture3D*>(this));

    if (!IsCompressed_OGL())
        glGetTexImage(target_, level, GetExternalFormat_OGL(format_), GetDataType_OGL(format_), dest);
    else
        glGetCompressedTexImage(target_, level, dest);

    graphics.SetTexture(0, nullptr);
    return true;
#else
    DV_LOGERROR("Getting texture data not supported");
    return false;
#endif
}

bool Texture3D::Create_OGL()
{
    Release_OGL();

#if defined(GL_ES_VERSION_2_0) && !defined(GL_ES_VERSION_3_0)
    DV_LOGERROR("Failed to create 3D texture, currently unsupported on OpenGL ES 2");
    return false;
#else
    if (GParams::is_headless() || !width_ || !height_ || !depth_)
        return false;

    Graphics& graphics = DV_GRAPHICS;

    if (graphics.IsDeviceLost())
    {
        DV_LOGWARNING("Texture creation while device is lost");
        return true;
    }

    unsigned format = GetSRGB() ? GetSRGBFormat_OGL(format_) : format_;
    unsigned externalFormat = GetExternalFormat_OGL(format_);
    unsigned dataType = GetDataType_OGL(format_);

    glGenTextures(1, &gpu_object_name_);

    // Ensure that our texture is bound to OpenGL texture unit 0
    graphics.SetTextureForUpdate_OGL(this);

    // If not compressed, create the initial level 0 texture with null data
    bool success = true;

    if (!IsCompressed_OGL())
    {
        glGetError();
        glTexImage3D(target_, 0, format, width_, height_, depth_, 0, externalFormat, dataType, nullptr);
        if (glGetError())
        {
            DV_LOGERROR("Failed to create 3D texture");
            success = false;
        }
    }

    // Set mipmapping
    levels_ = CheckMaxLevels(width_, height_, depth_, requestedLevels_);
    glTexParameteri(target_, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(target_, GL_TEXTURE_MAX_LEVEL, levels_ - 1);

    // Set initial parameters, then unbind the texture
    UpdateParameters();
    graphics.SetTexture(0, nullptr);

    return success;
#endif
}

}
