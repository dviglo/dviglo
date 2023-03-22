// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../containers/ptr.h"

namespace dviglo
{

using GLuint = unsigned int;

class Graphics;

/// Base class for GPU resources.
class DV_API GPUObject
{
public:
    /// Construct with graphics subsystem pointer.
    explicit GPUObject();
    /// Destruct. Remove from the Graphics.
    virtual ~GPUObject();

    /// Mark the GPU resource destroyed on graphics context destruction.
    virtual void OnDeviceLost();
    /// Recreate the GPU resource and restore data if applicable.
    virtual void OnDeviceReset();
    /// Unconditionally release the GPU resource.
    virtual void Release();

    /// Clear the data lost flag.
    void ClearDataLost();

    /// Возвращает числовой идентификатор, который ассоциирован с объектом OpenGL
    u32 gpu_object_name() const { return gpu_object_name_; }

    /// Return whether data is lost due to context loss.
    bool IsDataLost() const { return dataLost_; }
    /// Return whether has pending data assigned while graphics context was lost.
    bool HasPendingData() const { return dataPending_; }

protected:
    /// Числовой идентификатор, который ассоциирован с объектом OpenGL
    GLuint gpu_object_name_ = 0;

    /// Data lost flag.
    bool dataLost_{};
    /// Data pending flag.
    bool dataPending_{};
};

} // namespace dviglo
