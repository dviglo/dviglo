// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "vertex_buffer.h"

#include "../graphics/graphics.h"
#include "../io/log.h"
#include "../math/math_defs.h"
#include "graphics_impl.h"
#include "vertex_buffer.h"

#include "../common/debug_new.h"

using namespace std;

namespace dviglo
{

VertexBuffer::VertexBuffer()
{
    UpdateOffsets();

    // Force shadowing mode if graphics subsystem does not exist
    if (GParams::is_headless())
        shadowed_ = true;
}

VertexBuffer::~VertexBuffer()
{
    Release();
}

void VertexBuffer::SetShadowed(bool enable)
{
    // If no graphics subsystem, can not disable shadowing
    if (GParams::is_headless())
        enable = true;

    if (enable != shadowed_)
    {
        if (enable && vertexSize_ && vertexCount_)
            shadowData_ = make_shared<byte[]>((size_t)vertexCount_ * vertexSize_);
        else
            shadowData_.reset();

        shadowed_ = enable;
    }
}

bool VertexBuffer::SetSize(i32 vertexCount, VertexElements elementMask, bool dynamic)
{
    assert(vertexCount >= 0);
    return SetSize(vertexCount, GetElements(elementMask), dynamic);
}

bool VertexBuffer::SetSize(i32 vertexCount, const Vector<VertexElement>& elements, bool dynamic)
{
    assert(vertexCount >= 0);
    Unlock();

    vertexCount_ = vertexCount;
    elements_ = elements;
    dynamic_ = dynamic;

    UpdateOffsets();

    if (shadowed_ && vertexCount_ && vertexSize_)
        shadowData_ = make_shared<byte[]>((size_t)vertexCount_ * vertexSize_);
    else
        shadowData_.reset();

    return Create();
}

void VertexBuffer::UpdateOffsets()
{
    i32 elementOffset = 0;
    elementHash_ = 0;
    elementMask_ = VertexElements::None;

    for (VertexElement& element : elements_)
    {
        element.offset_ = elementOffset;
        elementOffset += ELEMENT_TYPESIZES[element.type_];
        elementHash_ <<= 6;
        elementHash_ += (u64)(((i32)element.type_ + 1) * ((i32)element.semantic_ + 1) + element.index_);

        for (i32 j = 0; j < MAX_LEGACY_VERTEX_ELEMENTS; ++j)
        {
            const VertexElement& legacy = LEGACY_VERTEXELEMENTS[j];
            if (element.type_ == legacy.type_ && element.semantic_ == legacy.semantic_ && element.index_ == legacy.index_)
                elementMask_ |= VertexElements{1u << j};
        }
    }

    vertexSize_ = elementOffset;
}

const VertexElement* VertexBuffer::GetElement(VertexElementSemantic semantic, i8 index/* = 0*/) const
{
    assert(index >= 0);

    for (const VertexElement& element : elements_)
    {
        if (element.semantic_ == semantic && element.index_ == index)
            return &element;
    }

    return nullptr;
}

const VertexElement* VertexBuffer::GetElement(VertexElementType type, VertexElementSemantic semantic, i8 index/* = 0*/) const
{
    assert(index >= 0);

    for (const VertexElement& element : elements_)
    {
        if (element.type_ == type && element.semantic_ == semantic && element.index_ == index)
            return &element;
    }

    return nullptr;
}

const VertexElement* VertexBuffer::GetElement(const Vector<VertexElement>& elements, VertexElementType type, VertexElementSemantic semantic, i8 index/* = 0*/)
{
    assert(index >= 0);

    for (const VertexElement& element : elements)
    {
        if (element.type_ == type && element.semantic_ == semantic && element.index_ == index)
            return &element;
    }

    return nullptr;
}

bool VertexBuffer::HasElement(const Vector<VertexElement>& elements, VertexElementType type, VertexElementSemantic semantic, i8 index/* = 0*/)
{
    assert(index >= 0);
    return GetElement(elements, type, semantic, index) != nullptr;
}

i32 VertexBuffer::GetElementOffset(const Vector<VertexElement>& elements, VertexElementType type, VertexElementSemantic semantic, i8 index/* = 0*/)
{
    assert(index >= 0);
    const VertexElement* element = GetElement(elements, type, semantic, index);
    return element ? element->offset_ : NINDEX;
}

Vector<VertexElement> VertexBuffer::GetElements(VertexElements elementMask)
{
    Vector<VertexElement> ret;

    for (i32 i = 0; i < MAX_LEGACY_VERTEX_ELEMENTS; ++i)
    {
        if (!!(elementMask & VertexElements{1u << i}))
            ret.Push(LEGACY_VERTEXELEMENTS[i]);
    }

    return ret;
}

i32 VertexBuffer::GetVertexSize(const Vector<VertexElement>& elements)
{
    i32 size = 0;

    for (i32 i = 0; i < elements.Size(); ++i)
        size += ELEMENT_TYPESIZES[elements[i].type_];

    return size;
}

i32 VertexBuffer::GetVertexSize(VertexElements elementMask)
{
    i32 size = 0;

    for (i32 i = 0; i < MAX_LEGACY_VERTEX_ELEMENTS; ++i)
    {
        if (!!(elementMask & VertexElements{1u << i}))
            size += ELEMENT_TYPESIZES[LEGACY_VERTEXELEMENTS[i].type_];
    }

    return size;
}

void VertexBuffer::UpdateOffsets(Vector<VertexElement>& elements)
{
    i32 elementOffset = 0;

    for (VertexElement& element : elements)
    {
        element.offset_ = elementOffset;
        elementOffset += ELEMENT_TYPESIZES[element.type_];
    }
}

void VertexBuffer::OnDeviceLost()
{
    if (gpu_object_name_ && !DV_GRAPHICS.IsDeviceLost())
        glDeleteBuffers(1, &gpu_object_name_);

    GpuObject::OnDeviceLost();
}

void VertexBuffer::OnDeviceReset()
{
    if (!gpu_object_name_)
    {
        Create();
        dataLost_ = !UpdateToGPU();
    }
    else if (dataPending_)
    {
        dataLost_ = !UpdateToGPU();
    }

    dataPending_ = false;
}

void VertexBuffer::Release()
{
    Unlock();

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

bool VertexBuffer::SetData(const void* data)
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

    if (shadowData_ && data != shadowData_.get())
        memcpy(shadowData_.get(), data, (size_t)vertexCount_ * vertexSize_);

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

bool VertexBuffer::SetDataRange(const void* data, i32 start, i32 count, bool discard)
{
    assert(start >= 0 && count >= 0);

    if (start == 0 && count == vertexCount_)
        return SetData(data);

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

    byte* dst = shadowData_.get() + (intptr_t)start * vertexSize_;
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

void* VertexBuffer::Lock(i32 start, i32 count, bool discard)
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
        return shadowData_.get() + (intptr_t)start * vertexSize_;
    }
    else if (!GParams::is_headless())
    {
        lockState_ = LOCK_SCRATCH;
        lockScratchData_ = DV_GRAPHICS.ReserveScratchBuffer(count * vertexSize_);
        return lockScratchData_;
    }
    else
    {
        return nullptr;
    }
}

void VertexBuffer::Unlock()
{
    switch (lockState_)
    {
    case LOCK_SHADOW:
        SetDataRange(shadowData_.get() + (intptr_t)lockStart_ * vertexSize_, lockStart_, lockCount_, discardLock_);
        lockState_ = LOCK_NONE;
        break;

    case LOCK_SCRATCH:
        SetDataRange(lockScratchData_, lockStart_, lockCount_, discardLock_);
        if (!GParams::is_headless())
            DV_GRAPHICS.FreeScratchBuffer(lockScratchData_);
        lockScratchData_ = nullptr;
        lockState_ = LOCK_NONE;
        break;

    default:
        break;
    }
}

bool VertexBuffer::Create()
{
    if (!vertexCount_ || !elementMask_)
    {
        Release();
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

bool VertexBuffer::UpdateToGPU()
{
    if (gpu_object_name_ && shadowData_)
        return SetData(shadowData_.get());
    else
        return false;
}

} // namespace dviglo
