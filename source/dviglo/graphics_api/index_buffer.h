// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"
#include "gpu_object.h"
#include "graphics_defs.h"

namespace dviglo
{

/// Hardware index buffer.
class DV_API IndexBuffer : public Object, public GpuObject
{
    DV_OBJECT(IndexBuffer);

public:
    explicit IndexBuffer();
    /// Destruct.
    ~IndexBuffer() override;

    /// Mark the buffer destroyed on graphics context destruction. May be a no-op depending on the API.
    void OnDeviceLost() override;
    /// Recreate the buffer and restore data if applicable. May be a no-op depending on the API.
    void OnDeviceReset() override;
    /// Release buffer.
    void Release() override;

    /// Enable shadowing in CPU memory. Shadowing is forced on if the graphics subsystem does not exist.
    void SetShadowed(bool enable);
    /// Set size and vertex elements and dynamic mode. Previous data will be lost.
    bool SetSize(i32 indexCount, bool largeIndices, bool dynamic = false);
    /// Set all data in the buffer.
    bool SetData(const void* data);
    /// Set a data range in the buffer. Optionally discard data outside the range.
    bool SetDataRange(const void* data, i32 start, i32 count, bool discard = false);
    /// Lock the buffer for write-only editing. Return data pointer if successful. Optionally discard data outside the range.
    void* Lock(i32 start, i32 count, bool discard = false);
    /// Unlock the buffer and apply changes to the GPU buffer.
    void Unlock();

    /// Return whether CPU memory shadowing is enabled.
    bool IsShadowed() const { return shadowed_; }

    /// Return whether is dynamic.
    bool IsDynamic() const { return dynamic_; }

    /// Return whether is currently locked.
    bool IsLocked() const { return lockState_ != LOCK_NONE; }

    /// Return number of indices.
    i32 GetIndexCount() const { return indexCount_; }

    /// Return index size in bytes.
    i32 GetIndexSize() const { return indexSize_; }

    /// Return used vertex range from index range.
    bool GetUsedVertexRange(i32 start, i32 count, i32& minVertex, i32& vertexCount);

    /// Return CPU memory shadow data.
    byte* GetShadowData() const { return shadowData_.get(); }

    /// Return shared array pointer to the CPU memory shadow data.
    std::shared_ptr<byte[]> GetShadowDataShared() const { return shadowData_; }

private:
    /// Create buffer.
    bool Create();
    /// Update the shadow data to the GPU buffer.
    bool UpdateToGPU();

    /// Shadow data.
    std::shared_ptr<byte[]> shadowData_;
    /// Number of indices.
    i32 indexCount_;
    /// Index size.
    i32 indexSize_;
    /// Buffer locking state.
    LockState lockState_;
    /// Lock start vertex.
    i32 lockStart_;
    /// Lock number of vertices.
    i32 lockCount_;
    /// Scratch buffer for fallback locking.
    void* lockScratchData_;
    /// Dynamic flag.
    bool dynamic_;
    /// Shadowed flag.
    bool shadowed_;
    /// Discard lock flag. Used by OpenGL only.
    bool discardLock_;
};

}
