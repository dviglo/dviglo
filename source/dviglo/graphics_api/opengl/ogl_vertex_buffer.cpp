// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../../graphics/graphics.h"
#include "../graphics_impl.h"
#include "../vertex_buffer.h"
#include "../../io/log.h"

#include "../../common/debug_new.h"

namespace dviglo
{

void VertexBuffer::OnDeviceLost_OGL()
{
    if (gpu_object_name_ && !DV_GRAPHICS.IsDeviceLost())
        glDeleteBuffers(1, &gpu_object_name_);

    GpuObject::OnDeviceLost();
}

void VertexBuffer::OnDeviceReset_OGL()
{
    if (!gpu_object_name_)
    {
        Create_OGL();
        dataLost_ = !UpdateToGPU_OGL();
    }
    else if (dataPending_)
    {
        dataLost_ = !UpdateToGPU_OGL();
    }

    dataPending_ = false;
}

void VertexBuffer::Release_OGL()
{
    Unlock_OGL();

    if (gpu_object_name_)
    {
        if (GParams::is_headless())
            return;

        Graphics& graphics = DV_GRAPHICS;

        if (!graphics.IsDeviceLost())
        {
            for (i32 i = 0; i < MAX_VERTEX_STREAMS; ++i)
            {
                if (graphics.GetVertexBuffer(i) == this)
                    graphics.SetVertexBuffer(nullptr);
            }

            graphics.SetVBO_OGL(0);
            glDeleteBuffers(1, &gpu_object_name_);
        }

        gpu_object_name_ = 0;
    }
}

bool VertexBuffer::SetData_OGL(const void* data)
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

    if (gpu_object_name_)
    {
        if (!DV_GRAPHICS.IsDeviceLost())
        {
            DV_GRAPHICS.SetVBO_OGL(gpu_object_name_);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vertexCount_ * vertexSize_, data, dynamic_ ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        }
        else
        {
            DV_LOGWARNING("Vertex buffer data assignment while device is lost");
            dataPending_ = true;
        }
    }

    dataLost_ = false;
    return true;
}

bool VertexBuffer::SetDataRange_OGL(const void* data, i32 start, i32 count, bool discard)
{
    assert(start >= 0 && count >= 0);

    if (start == 0 && count == vertexCount_)
        return SetData_OGL(data);

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

    if (gpu_object_name_)
    {
        if (!DV_GRAPHICS.IsDeviceLost())
        {
            DV_GRAPHICS.SetVBO_OGL(gpu_object_name_);
            if (!discard || start != 0)
                glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)start * vertexSize_, (GLsizeiptr)count * vertexSize_, data);
            else
                glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)count * vertexSize_, data, dynamic_ ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        }
        else
        {
            DV_LOGWARNING("Vertex buffer data assignment while device is lost");
            dataPending_ = true;
        }
    }

    return true;
}

void* VertexBuffer::Lock_OGL(i32 start, i32 count, bool discard)
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
    discardLock_ = discard;

    if (shadowData_)
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

void VertexBuffer::Unlock_OGL()
{
    switch (lockState_)
    {
    case LOCK_SHADOW:
        SetDataRange_OGL(shadowData_.Get() + (intptr_t)lockStart_ * vertexSize_, lockStart_, lockCount_, discardLock_);
        lockState_ = LOCK_NONE;
        break;

    case LOCK_SCRATCH:
        SetDataRange_OGL(lockScratchData_, lockStart_, lockCount_, discardLock_);
        if (!GParams::is_headless())
            DV_GRAPHICS.FreeScratchBuffer(lockScratchData_);
        lockScratchData_ = nullptr;
        lockState_ = LOCK_NONE;
        break;

    default:
        break;
    }
}

bool VertexBuffer::Create_OGL()
{
    if (!vertexCount_ || !elementMask_)
    {
        Release_OGL();
        return true;
    }

    if (!GParams::is_headless())
    {
        if (DV_GRAPHICS.IsDeviceLost())
        {
            DV_LOGWARNING("Vertex buffer creation while device is lost");
            return true;
        }

        if (!gpu_object_name_)
            glGenBuffers(1, &gpu_object_name_);

        if (!gpu_object_name_)
        {
            DV_LOGERROR("Failed to create vertex buffer");
            return false;
        }

        DV_GRAPHICS.SetVBO_OGL(gpu_object_name_);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vertexCount_ * vertexSize_, nullptr, dynamic_ ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    }

    return true;
}

bool VertexBuffer::UpdateToGPU_OGL()
{
    if (gpu_object_name_ && shadowData_)
        return SetData_OGL(shadowData_.Get());
    else
        return false;
}

}
