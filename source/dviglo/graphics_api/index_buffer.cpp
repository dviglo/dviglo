// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "index_buffer.h"

#include "../core/context.h"
#include "../graphics/graphics.h"
#include "../io/log.h"
#include "graphics_impl.h"

#include "../common/debug_new.h"

namespace dviglo
{

IndexBuffer::IndexBuffer() :
    indexCount_(0),
    indexSize_(0),
    lockState_(LOCK_NONE),
    lockStart_(0),
    lockCount_(0),
    lockScratchData_(nullptr),
    shadowed_(false),
    dynamic_(false),
    discardLock_(false)
{
    // Force shadowing mode if graphics subsystem does not exist
    if (GParams::is_headless())
        shadowed_ = true;
}

IndexBuffer::~IndexBuffer()
{
    Release();
}

void IndexBuffer::SetShadowed(bool enable)
{
    // If no graphics subsystem, can not disable shadowing
    if (GParams::is_headless())
        enable = true;

    if (enable != shadowed_)
    {
        if (enable && indexCount_ && indexSize_)
            shadowData_ = new byte[(size_t)indexCount_ * indexSize_];
        else
            shadowData_.Reset();

        shadowed_ = enable;
    }
}

bool IndexBuffer::SetSize(i32 indexCount, bool largeIndices, bool dynamic)
{
    assert(indexCount >= 0);
    Unlock();

    indexCount_ = indexCount;
    indexSize_ = (i32)(largeIndices ? sizeof(u32) : sizeof(u16));
    dynamic_ = dynamic;

    if (shadowed_ && indexCount_ && indexSize_)
        shadowData_ = new byte[(size_t)indexCount_ * indexSize_];
    else
        shadowData_.Reset();

    return Create();
}

bool IndexBuffer::GetUsedVertexRange(i32 start, i32 count, i32& minVertex, i32& vertexCount)
{
    assert(start >= 0 && count >= 0);

    if (!shadowData_)
    {
        DV_LOGERROR("Used vertex range can only be queried from an index buffer with shadow data");
        return false;
    }

    if (start + count > indexCount_)
    {
        DV_LOGERROR("Illegal index range for querying used vertices");
        return false;
    }

    minVertex = M_MAX_INT;
    i32 maxVertex = 0;

    if (indexSize_ == sizeof(u32))
    {
        u32* indices = (u32*)shadowData_.Get() + start;

        for (i32 i = 0; i < count; ++i)
        {
            i32 index = (i32)indices[i];

            if (index < minVertex)
                minVertex = index;

            if (index > maxVertex)
                maxVertex = index;
        }
    }
    else
    {
        u16* indices = (u16*)shadowData_.Get() + start;

        for (i32 i = 0; i < count; ++i)
        {
            i32 index = (i32)indices[i];

            if (index < minVertex)
                minVertex = index;

            if (index > maxVertex)
                maxVertex = index;
        }
    }

    vertexCount = maxVertex - minVertex + 1;
    return true;
}

void IndexBuffer::OnDeviceLost()
{
    if (gpu_object_name_ && !DV_GRAPHICS.IsDeviceLost())
        glDeleteBuffers(1, &gpu_object_name_);

    GpuObject::OnDeviceLost();
}

void IndexBuffer::OnDeviceReset()
{
    if (!gpu_object_name_)
    {
        Create_OGL();
        dataLost_ = !UpdateToGPU();
    }
    else if (dataPending_)
    {
        dataLost_ = !UpdateToGPU();
    }

    dataPending_ = false;
}

void IndexBuffer::Release()
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

bool IndexBuffer::SetData(const void* data)
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetData_OGL(data);
#endif

    return {}; // Prevent warning
}

bool IndexBuffer::SetDataRange(const void* data, i32 start, i32 count, bool discard)
{
    assert(start >= 0 && count >= 0);
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetDataRange_OGL(data, start, count, discard);
#endif

    return {}; // Prevent warning
}

void* IndexBuffer::Lock(i32 start, i32 count, bool discard)
{
    assert(start >= 0 && count >= 0);
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Lock_OGL(start, count, discard);
#endif

    return {}; // Prevent warning
}

void IndexBuffer::Unlock()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Unlock_OGL();
#endif
}

bool IndexBuffer::Create()
{
    GAPI gapi = GParams::get_gapi();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Create_OGL();
#endif

    return {}; // Prevent warning
}

bool IndexBuffer::UpdateToGPU()
{
    if (gpu_object_name_ && shadowData_)
        return SetData_OGL(shadowData_.Get());
    else
        return false;
}

} // namespace dviglo
