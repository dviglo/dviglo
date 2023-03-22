// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../../core/context.h"
#include "../../core/profiler.h"
#include "../../graphics/graphics.h"
#include "../../graphics/graphics_events.h"
#include "../../graphics/renderer.h"
#include "../graphics_impl.h"
#include "../texture_2d_array.h"
#include "../../io/file_system.h"
#include "../../io/log.h"
#include "../../resource/resource_cache.h"
#include "../../resource/xml_file.h"

#include "../../common/debug_new.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

namespace dviglo
{

void Texture2DArray::OnDeviceLost_OGL()
{
    if (gpu_object_name_ && !DV_GRAPHICS.IsDeviceLost())
        glDeleteTextures(1, &gpu_object_name_);

    GPUObject::OnDeviceLost();

    if (renderSurface_)
        renderSurface_->OnDeviceLost();
}

void Texture2DArray::OnDeviceReset_OGL()
{
    if (!gpu_object_name_ || dataPending_)
    {
        // If has a resource file, reload through the resource cache. Otherwise just recreate.
        if (DV_RES_CACHE.Exists(GetName()))
            dataLost_ = !DV_RES_CACHE.ReloadResource(this);

        if (!gpu_object_name_)
        {
            Create_OGL();
            dataLost_ = true;
        }
    }

    dataPending_ = false;
}

void Texture2DArray::Release_OGL()
{
    if (gpu_object_name_)
    {
        if (GParams::is_headless())
            return;

        Graphics& graphics = DV_GRAPHICS;

        if (!graphics.IsDeviceLost())
        {
            for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
            {
                if (graphics.GetTexture(i) == this)
                    graphics.SetTexture(i, nullptr);
            }

            glDeleteTextures(1, &gpu_object_name_);
        }

        if (renderSurface_)
            renderSurface_->Release();

        gpu_object_name_ = 0;
    }

    levelsDirty_ = false;
}

bool Texture2DArray::SetData_OGL(unsigned layer, unsigned level, int x, int y, int width, int height, const void* data)
{
    DV_PROFILE(SetTextureData);

    if (!gpu_object_name_ || GParams::is_headless())
    {
        DV_LOGERROR("Texture array not created, can not set data");
        return false;
    }

    if (!data)
    {
        DV_LOGERROR("Null source for setting data");
        return false;
    }

    if (layer >= layers_)
    {
        DV_LOGERROR("Illegal layer for setting data");
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
        DV_LOGWARNING("Texture array data assignment while device is lost");
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
    if (x < 0 || x + width > levelWidth || y < 0 || y + height > levelHeight || width <= 0 || height <= 0)
    {
        DV_LOGERROR("Illegal dimensions for setting data");
        return false;
    }

    graphics.SetTextureForUpdate_OGL(this);

#ifndef GL_ES_VERSION_2_0
    bool wholeLevel = x == 0 && y == 0 && width == levelWidth && height == levelHeight && layer == 0;
    unsigned format = GetSRGB() ? GetSRGBFormat_OGL(format_) : format_;

    if (!IsCompressed_OGL())
    {
        if (wholeLevel)
            glTexImage3D(target_, level, format, width, height, layers_, 0, GetExternalFormat_OGL(format_),
                GetDataType_OGL(format_), nullptr);
        glTexSubImage3D(target_, level, x, y, layer, width, height, 1, GetExternalFormat_OGL(format_),
            GetDataType_OGL(format_), data);
    }
    else
    {
        if (wholeLevel)
            glCompressedTexImage3D(target_, level, format, width, height, layers_, 0,
                GetDataSize(width, height, layers_), nullptr);
        glCompressedTexSubImage3D(target_, level, x, y, layer, width, height, 1, format,
            GetDataSize(width, height), data);
    }
#endif

    graphics.SetTexture(0, nullptr);
    return true;
}

bool Texture2DArray::SetData_OGL(unsigned layer, Deserializer& source)
{
    SharedPtr<Image> image(new Image());
    if (!image->Load(source))
        return false;

    return SetData_OGL(layer, image);
}

bool Texture2DArray::SetData_OGL(unsigned layer, Image* image, bool useAlpha)
{
    if (!image)
    {
        DV_LOGERROR("Null image, can not set data");
        return false;
    }
    if (!layers_)
    {
        DV_LOGERROR("Number of layers in the array must be set first");
        return false;
    }
    if (layer >= layers_)
    {
        DV_LOGERROR("Illegal layer for setting data");
        return false;
    }

    // Use a shared ptr for managing the temporary mip images created during this function
    SharedPtr<Image> mipImage;
    unsigned memoryUse = 0;
    MaterialQuality quality = QUALITY_HIGH;
    if (!GParams::is_headless())
        quality = DV_RENDERER.GetTextureQuality();

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
        unsigned format = 0;

        // Discard unnecessary mip levels
        for (unsigned i = 0; i < mipsToSkip_[quality]; ++i)
        {
            mipImage = image->GetNextLevel(); image = mipImage;
            levelData = image->GetData();
            levelWidth = image->GetWidth();
            levelHeight = image->GetHeight();
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

        // Create the texture array when layer 0 is being loaded, check that rest of the layers are same size & format
        if (!layer)
        {
            // If image was previously compressed, reset number of requested levels to avoid error if level count is too high for new size
            if (IsCompressed_OGL() && requestedLevels_ > 1)
                requestedLevels_ = 0;
            // Create the texture array (the number of layers must have been already set)
            SetSize(0, levelWidth, levelHeight, format);
        }
        else
        {
            if (!gpu_object_name_)
            {
                DV_LOGERROR("Texture array layer 0 must be loaded first");
                return false;
            }
            if (levelWidth != width_ || levelHeight != height_ || format != format_)
            {
                DV_LOGERROR("Texture array layer does not match size or format of layer 0");
                return false;
            }
        }

        for (unsigned i = 0; i < levels_; ++i)
        {
            SetData_OGL(layer, i, 0, 0, levelWidth, levelHeight, levelData);
            memoryUse += levelWidth * levelHeight * components;

            if (i < levels_ - 1)
            {
                mipImage = image->GetNextLevel(); image = mipImage;
                levelData = image->GetData();
                levelWidth = image->GetWidth();
                levelHeight = image->GetHeight();
            }
        }
    }
    else
    {
        int width = image->GetWidth();
        int height = image->GetHeight();
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
        while (mipsToSkip && (width / (1u << mipsToSkip) < 4 || height / (1u << mipsToSkip) < 4))
            --mipsToSkip;
        width /= (1u << mipsToSkip);
        height /= (1u << mipsToSkip);

        // Create the texture array when layer 0 is being loaded, assume rest of the layers are same size & format
        if (!layer)
        {
            SetNumLevels(Max((levels - mipsToSkip), 1U));
            SetSize(0, width, height, format);
        }
        else
        {
            if (!gpu_object_name_)
            {
                DV_LOGERROR("Texture array layer 0 must be loaded first");
                return false;
            }
            if (width != width_ || height != height_ || format != format_)
            {
                DV_LOGERROR("Texture array layer does not match size or format of layer 0");
                return false;
            }
        }

        for (unsigned i = 0; i < levels_ && i < levels - mipsToSkip; ++i)
        {
            CompressedLevel level = image->GetCompressedLevel(i + mipsToSkip);
            if (!needDecompress)
            {
                SetData_OGL(layer, i, 0, 0, level.width_, level.height_, level.data_);
                memoryUse += level.rows_ * level.rowSize_;
            }
            else
            {
                auto* rgbaData = new unsigned char[level.width_ * level.height_ * 4];
                level.Decompress(rgbaData);
                SetData_OGL(layer, i, 0, 0, level.width_, level.height_, rgbaData);
                memoryUse += level.width_ * level.height_ * 4;
                delete[] rgbaData;
            }
        }
    }

    layerMemoryUse_[layer] = memoryUse;
    unsigned totalMemoryUse = sizeof(Texture2DArray) + layerMemoryUse_.Capacity() * sizeof(unsigned);
    for (unsigned i = 0; i < layers_; ++i)
        totalMemoryUse += layerMemoryUse_[i];
    SetMemoryUse(totalMemoryUse);

    return true;
}

bool Texture2DArray::GetData_OGL(unsigned layer, unsigned level, void* dest) const
{
#ifndef GL_ES_VERSION_2_0
    if (!gpu_object_name_ || GParams::is_headless())
    {
        DV_LOGERROR("Texture array not created, can not get data");
        return false;
    }

    if (!dest)
    {
        DV_LOGERROR("Null destination for getting data");
        return false;
    }

    if (layer != 0)
    {
        DV_LOGERROR("Only the full download of the array is supported, set layer=0");
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

    graphics.SetTextureForUpdate_OGL(const_cast<Texture2DArray*>(this));

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

bool Texture2DArray::Create_OGL()
{
    Release_OGL();

#if defined(GL_ES_VERSION_2_0) && !defined(GL_ES_VERSION_3_0)
    DV_LOGERROR("Failed to create 2D array texture, currently unsupported on OpenGL ES 2");
    return false;
#else

    if (GParams::is_headless() || !width_ || !height_ || !layers_)
        return false;

    Graphics& graphics = DV_GRAPHICS;

    if (graphics.IsDeviceLost())
    {
        DV_LOGWARNING("Texture array creation while device is lost");
        return true;
    }

    glGenTextures(1, &gpu_object_name_);

    // Ensure that our texture is bound to OpenGL texture unit 0
    graphics.SetTextureForUpdate_OGL(this);

    unsigned format = GetSRGB() ? GetSRGBFormat_OGL(format_) : format_;
    unsigned externalFormat = GetExternalFormat_OGL(format_);
    unsigned dataType = GetDataType_OGL(format_);

    // If not compressed, create the initial level 0 texture with null data
    bool success = true;
    if (!IsCompressed_OGL())
    {
        glGetError();
        glTexImage3D(target_, 0, format, width_, height_, layers_, 0, externalFormat, dataType, nullptr);
        if (glGetError())
            success = false;
    }
    if (!success)
        DV_LOGERROR("Failed to create texture array");

    // Set mipmapping
    if (usage_ == TEXTURE_DEPTHSTENCIL || usage_ == TEXTURE_DYNAMIC)
        requestedLevels_ = 1;
    else if (usage_ == TEXTURE_RENDERTARGET)
    {
#ifdef __EMSCRIPTEN__
        // glGenerateMipmap appears to not be working on WebGL, disable rendertarget mipmaps for now
        requestedLevels_ = 1;
#else
        if (requestedLevels_ != 1)
        {
            // Generate levels for the first time now
            RegenerateLevels_OGL();
            // Determine max. levels automatically
            requestedLevels_ = 0;
        }
#endif
    }

    levels_ = CheckMaxLevels(width_, height_, requestedLevels_);
    glTexParameteri(target_, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(target_, GL_TEXTURE_MAX_LEVEL, levels_ - 1);

    // Set initial parameters, then unbind the texture
    UpdateParameters();
    graphics.SetTexture(0, nullptr);

    return success;
#endif
}

}
