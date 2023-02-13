// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../../precompiled.h"

#include "../../core/context.h"
#include "../../graphics/graphics.h"
#include "../../graphics_api/graphics_impl.h"
#include "../../graphics_api/index_buffer.h"
#include "../../io/log.h"

#include "../../debug_new.h"

namespace dviglo
{

void IndexBuffer::OnDeviceLost_OGL()
{
    if (object_.name_ && !graphics_->IsDeviceLost())
        glDeleteBuffers(1, &object_.name_);

    GPUObject::OnDeviceLost();
}

void IndexBuffer::OnDeviceReset_OGL()
{
    if (!object_.name_)
    {
        Create_OGL();
        dataLost_ = !UpdateToGPU_OGL();
    }
    else if (dataPending_)
        dataLost_ = !UpdateToGPU_OGL();

    dataPending_ = false;
}

void IndexBuffer::Release_OGL()
{
    Unlock_OGL();

    if (object_.name_)
    {
        if (!graphics_)
            return;

        if (!graphics_->IsDeviceLost())
        {
            if (graphics_->GetIndexBuffer() == this)
                graphics_->SetIndexBuffer(nullptr);

            glDeleteBuffers(1, &object_.name_);
        }

        object_.name_ = 0;
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

    if (object_.name_)
    {
        if (!graphics_->IsDeviceLost())
        {
            graphics_->SetIndexBuffer(this);
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

    if (object_.name_)
    {
        if (!graphics_->IsDeviceLost())
        {
            graphics_->SetIndexBuffer(this);
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
    else if (graphics_)
    {
        lockState_ = LOCK_SCRATCH;
        lockScratchData_ = graphics_->ReserveScratchBuffer(count * indexSize_);
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
        if (graphics_)
            graphics_->FreeScratchBuffer(lockScratchData_);
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

    if (graphics_)
    {
        if (graphics_->IsDeviceLost())
        {
            DV_LOGWARNING("Index buffer creation while device is lost");
            return true;
        }

        if (!object_.name_)
            glGenBuffers(1, &object_.name_);

        if (!object_.name_)
        {
            DV_LOGERROR("Failed to create index buffer");
            return false;
        }

        graphics_->SetIndexBuffer(this);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)indexCount_ * indexSize_, nullptr, dynamic_ ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    }

    return true;
}

bool IndexBuffer::UpdateToGPU_OGL()
{
    if (object_.name_ && shadowData_)
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
