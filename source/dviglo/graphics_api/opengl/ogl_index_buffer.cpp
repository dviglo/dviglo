// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../../core/context.h"
#include "../../graphics/graphics.h"
#include "../graphics_impl.h"
#include "../index_buffer.h"
#include "../../io/log.h"

#include "../../common/debug_new.h"

namespace dviglo
{

void IndexBuffer::Release_OGL()
{
    Unlock_OGL();

    if (gpu_object_name_)
    {
        if (GParams::is_headless())
            return;

        Graphics& graphics = DV_GRAPHICS;

        if (!graphics.IsDeviceLost())
        {
            if (graphics.GetIndexBuffer() == this)
                graphics.SetIndexBuffer(nullptr);

            glDeleteBuffers(1, &gpu_object_name_);
        }

        gpu_object_name_ = 0;
    }
}

bool IndexBuffer::SetData_OGL(const void* data)
{
    if (!data)
    {
        DV_LOGERROR("Null pointer for index buffer data");
        return false;
    }

    if (!indexSize_)
    {
        DV_LOGERROR("Index size not defined, can not set index buffer data");
        return false;
    }

    if (shadowData_ && data != shadowData_.Get())
        memcpy(shadowData_.Get(), data, (size_t)indexCount_ * indexSize_);

    if (gpu_object_name_)
    {
        if (!DV_GRAPHICS.IsDeviceLost())
        {
            DV_GRAPHICS.SetIndexBuffer(this);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)indexCount_ * indexSize_, data, dynamic_ ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        }
        else
        {
            DV_LOGWARNING("Index buffer data assignment while device is lost");
            dataPending_ = true;
        }
    }

    dataLost_ = false;
    return true;
}

bool IndexBuffer::SetDataRange_OGL(const void* data, i32 start, i32 count, bool discard)
{
    assert(start >= 0 && count >= 0);

    if (start == 0 && count == indexCount_)
        return SetData_OGL(data);

    if (!data)
    {
        DV_LOGERROR("Null pointer for index buffer data");
        return false;
    }

    if (!indexSize_)
    {
        DV_LOGERROR("Index size not defined, can not set index buffer data");
        return false;
    }

    if (start + count > indexCount_)
    {
        DV_LOGERROR("Illegal range for setting new index buffer data");
        return false;
    }

    if (!count)
        return true;

    byte* dst = shadowData_.Get() + (intptr_t)start * indexSize_;
    if (shadowData_ && dst != data)
        memcpy(dst, data, (size_t)count * indexSize_);

    if (gpu_object_name_)
    {
        if (!DV_GRAPHICS.IsDeviceLost())
        {
            DV_GRAPHICS.SetIndexBuffer(this);
            if (!discard || start != 0)
                glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, (GLintptr)start * indexSize_, (GLsizeiptr)count * indexSize_, data);
            else
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)count * indexSize_, data, dynamic_ ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        }
        else
        {
            DV_LOGWARNING("Index buffer data assignment while device is lost");
            dataPending_ = true;
        }
    }

    return true;
}

void* IndexBuffer::Lock_OGL(i32 start, i32 count, bool discard)
{
    assert(start >= 0 && count >= 0);

    if (lockState_ != LOCK_NONE)
    {
        DV_LOGERROR("Index buffer already locked");
        return nullptr;
    }

    if (!indexSize_)
    {
        DV_LOGERROR("Index size not defined, can not lock index buffer");
        return nullptr;
    }

    if (start + count > indexCount_)
    {
        DV_LOGERROR("Illegal range for locking index buffer");
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
        return shadowData_.Get() + (intptr_t)start * indexSize_;
    }
    else if (!GParams::is_headless())
    {
        lockState_ = LOCK_SCRATCH;
        lockScratchData_ = DV_GRAPHICS.ReserveScratchBuffer(count * indexSize_);
        return lockScratchData_;
    }
    else
        return nullptr;
}

void IndexBuffer::Unlock_OGL()
{
    switch (lockState_)
    {
    case LOCK_SHADOW:
        SetDataRange_OGL(shadowData_.Get() + (intptr_t)lockStart_ * indexSize_, lockStart_, lockCount_, discardLock_);
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

bool IndexBuffer::Create_OGL()
{
    if (!indexCount_)
    {
        Release_OGL();
        return true;
    }

    if (!GParams::is_headless())
    {
        if (DV_GRAPHICS.IsDeviceLost())
        {
            DV_LOGWARNING("Index buffer creation while device is lost");
            return true;
        }

        if (!gpu_object_name_)
            glGenBuffers(1, &gpu_object_name_);

        if (!gpu_object_name_)
        {
            DV_LOGERROR("Failed to create index buffer");
            return false;
        }

        DV_GRAPHICS.SetIndexBuffer(this);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)indexCount_ * indexSize_, nullptr, dynamic_ ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    }

    return true;
}

bool IndexBuffer::UpdateToGPU_OGL()
{
    if (gpu_object_name_ && shadowData_)
        return SetData_OGL(shadowData_.Get());
    else
        return false;
}

void* IndexBuffer::MapBuffer_OGL(i32 start, i32 count, bool discard)
{
    // Never called on OpenGL
    return nullptr;
}

void IndexBuffer::UnmapBuffer_OGL()
{
    // Never called on OpenGL
}

} // namespace dviglo
