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

bool IndexBuffer::Create_OGL()
{
    if (!indexCount_)
    {
        Release();
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

} // namespace dviglo
