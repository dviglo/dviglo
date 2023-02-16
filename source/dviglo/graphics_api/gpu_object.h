// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../containers/ptr.h"

namespace dviglo
{

class Graphics;

/// API-specific GPU object representation
union GPUObjectHandle
{
    /// Object pointer (Direct3D)
    void* ptr_;

    /// Object name (OpenGL)
    u32 name_;
};

/// Base class for GPU resources.
class DV_API GPUObject
{
public:
    /// Construct with graphics subsystem pointer.
    explicit GPUObject(Graphics* graphics);
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

    /// Return the graphics subsystem associated with this GPU object.
    Graphics* GetGraphics() const;
    /// Return the object pointer. Applicable only on Direct3D.
    void* GetGPUObject() const { return object_.ptr_; }
    /// Return the object name. Applicable only on OpenGL.
    u32 GetGPUObjectName() const { return object_.name_; }
    /// Return whether data is lost due to context loss.
    bool IsDataLost() const { return dataLost_; }
    /// Return whether has pending data assigned while graphics context was lost.
    bool HasPendingData() const { return dataPending_; }

protected:
    /// Graphics subsystem.
    WeakPtr<Graphics> graphics_;
    /// Object pointer or name.
    GPUObjectHandle object_{};
    /// Data lost flag.
    bool dataLost_{};
    /// Data pending flag.
    bool dataPending_{};
};

}

