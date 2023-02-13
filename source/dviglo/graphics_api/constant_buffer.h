// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../container/array_ptr.h"
#include "../core/object.h"
#include "../graphics_api/gpu_object.h"
#include "../graphics_api/graphics_defs.h"

namespace dviglo
{

/// Hardware constant buffer.
class DV_API ConstantBuffer : public Object, public GPUObject
{
    DV_OBJECT(ConstantBuffer, Object);

public:
    /// Construct.
    explicit ConstantBuffer(Context* context);
    /// Destruct.
    ~ConstantBuffer() override;

    /// Recreate the GPU resource and restore data if applicable.
    void OnDeviceReset() override;
    /// Release the buffer.
    void Release() override;

    /// Set size and create GPU-side buffer. Return true on success.
    bool SetSize(unsigned size);
    /// Set a generic parameter and mark buffer dirty.
    void SetParameter(unsigned offset, unsigned size, const void* data);
    /// Set a Vector3 array parameter and mark buffer dirty.
    void SetVector3ArrayParameter(unsigned offset, unsigned rows, const void* data);
    /// Apply to GPU.
    void Apply();

    /// Return size.
    unsigned GetSize() const { return size_; }

    /// Return whether has unapplied data.
    bool IsDirty() const { return dirty_; }

private:
#ifdef DV_OPENGL
    void Release_OGL();
    void OnDeviceReset_OGL();
    bool SetSize_OGL(unsigned size);
    void Apply_OGL();
#endif // def DV_OPENGL

#ifdef DV_D3D11
    void Release_D3D11();
    void OnDeviceReset_D3D11();
    bool SetSize_D3D11(unsigned size);
    void Apply_D3D11();
#endif // def DV_D3D11

    /// Shadow data.
    SharedArrayPtr<unsigned char> shadowData_;
    /// Buffer byte size.
    unsigned size_{};
    /// Dirty flag.
    bool dirty_{};
};

}
