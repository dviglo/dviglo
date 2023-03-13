// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../../io/log.h"
#include "../vertex_buffer.h"
#include "../../graphics/graphics.h"
#include "d3d11_graphics_impl.h"

#include "../../common/debug_new.h"

namespace dviglo
{

void VertexBuffer::OnDeviceLost_D3D11()
{
    // No-op on Direct3D11
}

void VertexBuffer::OnDeviceReset_D3D11()
{
    // No-op on Direct3D11
}

void VertexBuffer::Release_D3D11()
{
    Unlock_D3D11();

    if (!GParams::is_headless())
    {
        Graphics& graphics = DV_GRAPHICS;

        for (i32 i = 0; i < MAX_VERTEX_STREAMS; ++i)
        {
            if (graphics.GetVertexBuffer(i) == this)
                graphics.SetVertexBuffer(nullptr);
        }
    }

    DV_SAFE_RELEASE(object_.ptr_);
}

bool VertexBuffer::SetData_D3D11(const void* data)
{
    if (!data)
    {
        DV_LOGERROR("Null pointer for vertex buffer data");
        return false;
    }

    if (!vertexSize_)
    {
        DV_LOGERROR("Vertex elements not defined, can not set vertex buffer data");
        return false;
    }

    if (shadowData_ && data != shadowData_.Get())
        memcpy(shadowData_.Get(), data, (size_t)vertexCount_ * vertexSize_);

    if (object_.ptr_)
    {
        if (dynamic_)
        {
            void* hwData = MapBuffer_D3D11(0, vertexCount_, true);
            if (hwData)
            {
                memcpy(hwData, data, (size_t)vertexCount_ * vertexSize_);
                UnmapBuffer_D3D11();
            }
            else
                return false;
        }
        else
        {
            D3D11_BOX destBox;
            destBox.left = 0;
            destBox.right = (UINT)vertexCount_ * vertexSize_;
            destBox.top = 0;
            destBox.bottom = 1;
            destBox.front = 0;
            destBox.back = 1;

            DV_GRAPHICS.GetImpl_D3D11()->GetDeviceContext()->UpdateSubresource((ID3D11Buffer*)object_.ptr_, 0, &destBox, data, 0, 0);
        }
    }

    return true;
}

bool VertexBuffer::SetDataRange_D3D11(const void* data, i32 start, i32 count, bool discard)
{
    assert(start >= 0 && count >= 0);

    if (start == 0 && count == vertexCount_)
        return SetData_D3D11(data);

    if (!data)
    {
        DV_LOGERROR("Null pointer for vertex buffer data");
        return false;
    }

    if (!vertexSize_)
    {
        DV_LOGERROR("Vertex elements not defined, can not set vertex buffer data");
        return false;
    }

    if (start + count > vertexCount_)
    {
        DV_LOGERROR("Illegal range for setting new vertex buffer data");
        return false;
    }

    if (!count)
        return true;

    byte* dst = shadowData_.Get() + (intptr_t)start * vertexSize_;
    if (shadowData_ && dst != data)
        memcpy(dst, data, (size_t)count * vertexSize_);

    if (object_.ptr_)
    {
        if (dynamic_)
        {
            void* hwData = MapBuffer_D3D11(start, count, discard);
            if (hwData)
            {
                memcpy(hwData, data, (size_t)count * vertexSize_);
                UnmapBuffer_D3D11();
            }
            else
                return false;
        }
        else
        {
            D3D11_BOX destBox;
            destBox.left = (UINT)start * vertexSize_;
            destBox.right = destBox.left + (UINT)count * vertexSize_;
            destBox.top = 0;
            destBox.bottom = 1;
            destBox.front = 0;
            destBox.back = 1;

            DV_GRAPHICS.GetImpl_D3D11()->GetDeviceContext()->UpdateSubresource((ID3D11Buffer*)object_.ptr_, 0, &destBox, data, 0, 0);
        }
    }

    return true;
}

void* VertexBuffer::Lock_D3D11(i32 start, i32 count, bool discard)
{
    assert(start >= 0 && count >= 0);

    if (lockState_ != LOCK_NONE)
    {
        DV_LOGERROR("Vertex buffer already locked");
        return nullptr;
    }

    if (!vertexSize_)
    {
        DV_LOGERROR("Vertex elements not defined, can not lock vertex buffer");
        return nullptr;
    }

    if (start + count > vertexCount_)
    {
        DV_LOGERROR("Illegal range for locking vertex buffer");
        return nullptr;
    }

    if (!count)
        return nullptr;

    lockStart_ = start;
    lockCount_ = count;

    // Because shadow data must be kept in sync, can only lock hardware buffer if not shadowed
    if (object_.ptr_ && !shadowData_ && dynamic_)
        return MapBuffer_D3D11(start, count, discard);
    else if (shadowData_)
    {
        lockState_ = LOCK_SHADOW;
        return shadowData_.Get() + (intptr_t)start * vertexSize_;
    }
    else if (!GParams::is_headless())
    {
        lockState_ = LOCK_SCRATCH;
        lockScratchData_ = DV_GRAPHICS.ReserveScratchBuffer(count * vertexSize_);
        return lockScratchData_;
    }
    else
        return nullptr;
}

void VertexBuffer::Unlock_D3D11()
{
    switch (lockState_)
    {
    case LOCK_HARDWARE:
        UnmapBuffer_D3D11();
        break;

    case LOCK_SHADOW:
        SetDataRange_D3D11(shadowData_.Get() + (intptr_t)lockStart_ * vertexSize_, lockStart_, lockCount_);
        lockState_ = LOCK_NONE;
        break;

    case LOCK_SCRATCH:
        SetDataRange_D3D11(lockScratchData_, lockStart_, lockCount_);
        if (!GParams::is_headless())
            DV_GRAPHICS.FreeScratchBuffer(lockScratchData_);
        lockScratchData_ = nullptr;
        lockState_ = LOCK_NONE;
        break;

    default: break;
    }
}

bool VertexBuffer::Create_D3D11()
{
    Release_D3D11();

    if (!vertexCount_ || !elementMask_)
        return true;

    if (!GParams::is_headless())
    {
        D3D11_BUFFER_DESC bufferDesc;
        memset(&bufferDesc, 0, sizeof bufferDesc);
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = dynamic_ ? D3D11_CPU_ACCESS_WRITE : 0;
        bufferDesc.Usage = dynamic_ ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = (UINT)vertexCount_ * vertexSize_;

        HRESULT hr = DV_GRAPHICS.GetImpl_D3D11()->GetDevice()->CreateBuffer(&bufferDesc, nullptr, (ID3D11Buffer**)&object_.ptr_);
        if (FAILED(hr))
        {
            DV_SAFE_RELEASE(object_.ptr_);
            DV_LOGD3DERROR("Failed to create vertex buffer", hr);
            return false;
        }
    }

    return true;
}

bool VertexBuffer::UpdateToGPU_D3D11()
{
    if (object_.ptr_ && shadowData_)
        return SetData_D3D11(shadowData_.Get());
    else
        return false;
}

void* VertexBuffer::MapBuffer_D3D11(i32 start, i32 count, bool discard)
{
    assert(start >= 0 && count >= 0);
    void* hwData = nullptr;

    if (object_.ptr_)
    {
        D3D11_MAPPED_SUBRESOURCE mappedData;
        mappedData.pData = nullptr;

        HRESULT hr = DV_GRAPHICS.GetImpl_D3D11()->GetDeviceContext()->Map((ID3D11Buffer*)object_.ptr_, 0, discard ? D3D11_MAP_WRITE_DISCARD :
            D3D11_MAP_WRITE, 0, &mappedData);
        if (FAILED(hr) || !mappedData.pData)
            DV_LOGD3DERROR("Failed to map vertex buffer", hr);
        else
        {
            hwData = mappedData.pData;
            lockState_ = LOCK_HARDWARE;
        }
    }

    return hwData;
}

void VertexBuffer::UnmapBuffer_D3D11()
{
    if (object_.ptr_ && lockState_ == LOCK_HARDWARE)
    {
        DV_GRAPHICS.GetImpl_D3D11()->GetDeviceContext()->Unmap((ID3D11Buffer*)object_.ptr_, 0);
        lockState_ = LOCK_NONE;
    }
}

}
