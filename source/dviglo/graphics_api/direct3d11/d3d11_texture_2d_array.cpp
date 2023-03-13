// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../../core/context.h"
#include "../../core/profiler.h"
#include "../../graphics/graphics.h"
#include "../../graphics/graphics_events.h"
#include "../../graphics/renderer.h"
#include "d3d11_graphics_impl.h"
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

void Texture2DArray::OnDeviceLost_D3D11()
{
    // No-op on Direct3D11
}

void Texture2DArray::OnDeviceReset_D3D11()
{
    // No-op on Direct3D11
}

void Texture2DArray::Release_D3D11()
{
    if (!GParams::is_headless())
    {
        Graphics& graphics = DV_GRAPHICS;

        for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
        {
            if (graphics.GetTexture(i) == this)
                graphics.SetTexture(i, nullptr);
        }
    }

    if (renderSurface_)
        renderSurface_->Release();

    DV_SAFE_RELEASE(object_.ptr_);
    DV_SAFE_RELEASE(shaderResourceView_);
    DV_SAFE_RELEASE(sampler_);

    levelsDirty_ = false;
}

bool Texture2DArray::SetData_D3D11(unsigned layer, unsigned level, int x, int y, int width, int height, const void* data)
{
    DV_PROFILE(SetTextureData);

    if (!object_.ptr_)
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

    int levelWidth = GetLevelWidth(level);
    int levelHeight = GetLevelHeight(level);
    if (x < 0 || x + width > levelWidth || y < 0 || y + height > levelHeight || width <= 0 || height <= 0)
    {
        DV_LOGERROR("Illegal dimensions for setting data");
        return false;
    }

    // If compressed, align the update region on a block
    if (IsCompressed_D3D11())
    {
        x &= ~3;
        y &= ~3;
        width += 3;
        width &= 0xfffffffc;
        height += 3;
        height &= 0xfffffffc;
    }

    unsigned char* src = (unsigned char*)data;
    unsigned rowSize = GetRowDataSize_D3D11(width);
    unsigned rowStart = GetRowDataSize_D3D11(x);
    unsigned subResource = D3D11CalcSubresource(level, layer, levels_);

    if (usage_ == TEXTURE_DYNAMIC)
    {
        if (IsCompressed_D3D11())
        {
            height = (height + 3) >> 2;
            y >>= 2;
        }

        D3D11_MAPPED_SUBRESOURCE mappedData;
        mappedData.pData = nullptr;

        HRESULT hr = DV_GRAPHICS.GetImpl_D3D11()->GetDeviceContext()->Map((ID3D11Resource*)object_.ptr_, subResource, D3D11_MAP_WRITE_DISCARD, 0,
            &mappedData);
        if (FAILED(hr) || !mappedData.pData)
        {
            DV_LOGD3DERROR("Failed to map texture for update", hr);
            return false;
        }
        else
        {
            for (int row = 0; row < height; ++row)
                memcpy((unsigned char*)mappedData.pData + (row + y) * mappedData.RowPitch + rowStart, src + row * rowSize, rowSize);
            DV_GRAPHICS.GetImpl_D3D11()->GetDeviceContext()->Unmap((ID3D11Resource*)object_.ptr_, subResource);
        }
    }
    else
    {
        D3D11_BOX destBox;
        destBox.left = (UINT)x;
        destBox.right = (UINT)(x + width);
        destBox.top = (UINT)y;
        destBox.bottom = (UINT)(y + height);
        destBox.front = 0;
        destBox.back = 1;

        DV_GRAPHICS.GetImpl_D3D11()->GetDeviceContext()->UpdateSubresource((ID3D11Resource*)object_.ptr_, subResource, &destBox, data,
            rowSize, 0);
    }

    return true;
}

bool Texture2DArray::SetData_D3D11(unsigned layer, Deserializer& source)
{
    SharedPtr<Image> image(new Image());
    if (!image->Load(source))
        return false;

    return SetData_D3D11(layer, image);
}

bool Texture2DArray::SetData_D3D11(unsigned layer, Image* image, bool useAlpha)
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
        if ((components == 1 && !useAlpha) || components == 2 || components == 3)
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
            format = Graphics::GetAlphaFormat();
            break;

        case 4:
            format = Graphics::GetRGBAFormat();
            break;

        default: break;
        }

        // Create the texture array when layer 0 is being loaded, check that rest of the layers are same size & format
        if (!layer)
        {
            // If image was previously compressed, reset number of requested levels to avoid error if level count is too high for new size
            if (IsCompressed_D3D11() && requestedLevels_ > 1)
                requestedLevels_ = 0;
            // Create the texture array (the number of layers must have been already set)
            SetSize(0, levelWidth, levelHeight, format);
        }
        else
        {
            if (!object_.ptr_)
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
            SetData_D3D11(layer, i, 0, 0, levelWidth, levelHeight, levelData);
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
        while (mipsToSkip && (width / (1 << mipsToSkip) < 4 || height / (1 << mipsToSkip) < 4))
            --mipsToSkip;
        width /= (1 << mipsToSkip);
        height /= (1 << mipsToSkip);

        // Create the texture array when layer 0 is being loaded, assume rest of the layers are same size & format
        if (!layer)
        {
            SetNumLevels(Max((levels - mipsToSkip), 1U));
            SetSize(0, width, height, format);
        }
        else
        {
            if (!object_.ptr_)
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
                SetData_D3D11(layer, i, 0, 0, level.width_, level.height_, level.data_);
                memoryUse += level.rows_ * level.rowSize_;
            }
            else
            {
                unsigned char* rgbaData = new unsigned char[level.width_ * level.height_ * 4];
                level.Decompress(rgbaData);
                SetData_D3D11(layer, i, 0, 0, level.width_, level.height_, rgbaData);
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

bool Texture2DArray::GetData_D3D11(unsigned layer, unsigned level, void* dest) const
{
    if (!object_.ptr_)
    {
        DV_LOGERROR("Texture array not created, can not get data");
        return false;
    }

    if (!dest)
    {
        DV_LOGERROR("Null destination for getting data");
        return false;
    }

    if (layer >= layers_)
    {
        DV_LOGERROR("Illegal layer for getting data");
        return false;
    }

    if (level >= levels_)
    {
        DV_LOGERROR("Illegal mip level for getting data");
        return false;
    }

    int levelWidth = GetLevelWidth(level);
    int levelHeight = GetLevelHeight(level);

    D3D11_TEXTURE2D_DESC textureDesc;
    memset(&textureDesc, 0, sizeof textureDesc);
    textureDesc.Width = (UINT)levelWidth;
    textureDesc.Height = (UINT)levelHeight;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = (DXGI_FORMAT)format_;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_STAGING;
    textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    Graphics& graphics = DV_GRAPHICS;

    ID3D11Texture2D* stagingTexture = nullptr;
    HRESULT hr = graphics.GetImpl_D3D11()->GetDevice()->CreateTexture2D(&textureDesc, nullptr, &stagingTexture);
    if (FAILED(hr))
    {
        DV_LOGD3DERROR("Failed to create staging texture for GetData", hr);
        DV_SAFE_RELEASE(stagingTexture);
        return false;
    }

    unsigned srcSubResource = D3D11CalcSubresource(level, layer, levels_);
    D3D11_BOX srcBox;
    srcBox.left = 0;
    srcBox.right = (UINT)levelWidth;
    srcBox.top = 0;
    srcBox.bottom = (UINT)levelHeight;
    srcBox.front = 0;
    srcBox.back = 1;
    graphics.GetImpl_D3D11()->GetDeviceContext()->CopySubresourceRegion(stagingTexture, 0, 0, 0, 0, (ID3D11Resource*)object_.ptr_,
        srcSubResource, &srcBox);

    D3D11_MAPPED_SUBRESOURCE mappedData;
    mappedData.pData = nullptr;
    unsigned rowSize = GetRowDataSize_D3D11(levelWidth);
    unsigned numRows = (unsigned)(IsCompressed_D3D11() ? (levelHeight + 3) >> 2 : levelHeight);

    hr = graphics.GetImpl_D3D11()->GetDeviceContext()->Map((ID3D11Resource*)stagingTexture, 0, D3D11_MAP_READ, 0, &mappedData);
    if (FAILED(hr) || !mappedData.pData)
    {
        DV_LOGD3DERROR("Failed to map staging texture for GetData", hr);
        DV_SAFE_RELEASE(stagingTexture);
        return false;
    }
    else
    {
        for (unsigned row = 0; row < numRows; ++row)
            memcpy((unsigned char*)dest + row * rowSize, (unsigned char*)mappedData.pData + row * mappedData.RowPitch, rowSize);
        graphics.GetImpl_D3D11()->GetDeviceContext()->Unmap((ID3D11Resource*)stagingTexture, 0);
        DV_SAFE_RELEASE(stagingTexture);
        return true;
    }
}

bool Texture2DArray::Create_D3D11()
{
    Release_D3D11();

    if (GParams::is_headless() || !width_ || !height_ || !layers_)
        return false;

    levels_ = CheckMaxLevels(width_, height_, requestedLevels_);

    D3D11_TEXTURE2D_DESC textureDesc;
    memset(&textureDesc, 0, sizeof textureDesc);

    // Set mipmapping
    if (usage_ == TEXTURE_RENDERTARGET && levels_ != 1)
        textureDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

    Graphics& graphics = DV_GRAPHICS;

    textureDesc.Width = (UINT)width_;
    textureDesc.Height = (UINT)height_;
    textureDesc.MipLevels = usage_ != TEXTURE_DYNAMIC ? levels_ : 1;
    textureDesc.ArraySize = layers_;
    textureDesc.Format = (DXGI_FORMAT)(sRGB_ ? GetSRGBFormat_D3D11(format_) : format_);
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = usage_ == TEXTURE_DYNAMIC ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (usage_ == TEXTURE_RENDERTARGET)
        textureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
    else if (usage_ == TEXTURE_DEPTHSTENCIL)
        textureDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
    textureDesc.CPUAccessFlags = usage_ == TEXTURE_DYNAMIC ? D3D11_CPU_ACCESS_WRITE : 0;

    HRESULT hr = graphics.GetImpl_D3D11()->GetDevice()->CreateTexture2D(&textureDesc, nullptr, (ID3D11Texture2D**)&object_);
    if (FAILED(hr))
    {
        DV_LOGD3DERROR("Failed to create texture array", hr);
        DV_SAFE_RELEASE(object_.ptr_);
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    memset(&srvDesc, 0, sizeof srvDesc);
    srvDesc.Format = (DXGI_FORMAT)GetSRVFormat_D3D11(textureDesc.Format);
    if (layers_ == 1)
    {
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = usage_ != TEXTURE_DYNAMIC ? (UINT)levels_ : 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
    }
    else
    {
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.MipLevels = usage_ != TEXTURE_DYNAMIC ? (UINT)levels_ : 1;
        srvDesc.Texture2DArray.ArraySize = layers_;
        srvDesc.Texture2DArray.FirstArraySlice = 0;
        srvDesc.Texture2DArray.MostDetailedMip = 0;
    }

    hr = graphics.GetImpl_D3D11()->GetDevice()->CreateShaderResourceView((ID3D11Resource*)object_.ptr_, &srvDesc,
        (ID3D11ShaderResourceView**)&shaderResourceView_);
    if (FAILED(hr))
    {
        DV_LOGD3DERROR("Failed to create shader resource view for texture array", hr);
        DV_SAFE_RELEASE(shaderResourceView_);
        return false;
    }

    if (usage_ == TEXTURE_RENDERTARGET)
    {
        D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
        memset(&renderTargetViewDesc, 0, sizeof renderTargetViewDesc);
        renderTargetViewDesc.Format = textureDesc.Format;
        if (layers_ == 1)
        {
            renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            renderTargetViewDesc.Texture2D.MipSlice = 0;
        }
        else
        {
            renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            renderTargetViewDesc.Texture2DArray.MipSlice = 0;
            renderTargetViewDesc.Texture2DArray.ArraySize = layers_;
            renderTargetViewDesc.Texture2DArray.FirstArraySlice = 0;
        }

        hr = graphics.GetImpl_D3D11()->GetDevice()->CreateRenderTargetView((ID3D11Resource*)object_.ptr_, &renderTargetViewDesc,
            (ID3D11RenderTargetView**)&renderSurface_->renderTargetView_);

        if (FAILED(hr))
        {
            DV_LOGD3DERROR("Failed to create rendertarget view for texture array", hr);
            DV_SAFE_RELEASE(renderSurface_->renderTargetView_);
            return false;
        }
    }

    return true;
}

}
