// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../graphics/graphics.h"
#include "../graphics_api/constant_buffer.h"
#include "../io/log.h"

#include "../debug_new.h"

namespace dviglo
{

ConstantBuffer::ConstantBuffer(Context* context) :
    Object(context),
    GPUObject(GetSubsystem<Graphics>())
{
}

ConstantBuffer::~ConstantBuffer()
{
    Release();
}

void ConstantBuffer::SetParameter(unsigned offset, unsigned size, const void* data)
{
    if (offset + size > size_)
        return; // Would overflow the buffer

    memcpy(&shadowData_[offset], data, size);
    dirty_ = true;
}

void ConstantBuffer::SetVector3ArrayParameter(unsigned offset, unsigned rows, const void* data)
{
    if (offset + rows * 4 * sizeof(float) > size_)
        return; // Would overflow the buffer

    auto* dest = (float*)&shadowData_[offset];
    const auto* src = (const float*)data;

    while (rows--)
    {
        *dest++ = *src++;
        *dest++ = *src++;
        *dest++ = *src++;
        ++dest; // Skip over the w coordinate
    }

    dirty_ = true;
}

void ConstantBuffer::Release()
{
    GAPI gapi = Graphics::GetGAPI();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Release_OGL();
#endif

#ifdef DV_D3D11
    if (gapi == GAPI_D3D11)
        return Release_D3D11();
#endif
}

void ConstantBuffer::OnDeviceReset()
{
    GAPI gapi = Graphics::GetGAPI();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return OnDeviceReset_OGL();
#endif

#ifdef DV_D3D11
    if (gapi == GAPI_D3D11)
        return OnDeviceReset_D3D11();
#endif
}

bool ConstantBuffer::SetSize(unsigned size)
{
    GAPI gapi = Graphics::GetGAPI();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return SetSize_OGL(size);
#endif

#ifdef DV_D3D11
    if (gapi == GAPI_D3D11)
        return SetSize_D3D11(size);
#endif

    return {}; // Prevent warning
}

void ConstantBuffer::Apply()
{
    GAPI gapi = Graphics::GetGAPI();

#ifdef DV_OPENGL
    if (gapi == GAPI_OPENGL)
        return Apply_OGL();
#endif

#ifdef DV_D3D11
    if (gapi == GAPI_D3D11)
        return Apply_D3D11();
#endif
}

}
