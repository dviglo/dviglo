// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "gpu_object.h"
#include "../graphics/graphics.h"

#include "../common/debug_new.h"

namespace dviglo
{

GPUObject::GPUObject()
{
    if (GParams::get_gapi() == GAPI_OPENGL)
        object_.name_ = 0;
    else
        object_.ptr_ = nullptr;

    if (!GParams::is_headless())
        DV_GRAPHICS.AddGPUObject(this);
}

GPUObject::~GPUObject()
{
    if (!GParams::is_headless())
        DV_GRAPHICS.RemoveGPUObject(this);
}

void GPUObject::OnDeviceLost()
{
    if (GParams::get_gapi() == GAPI_OPENGL)
    {
        // On OpenGL the object has already been lost at this point; reset object name
        object_.name_ = 0;
    }
}

void GPUObject::OnDeviceReset()
{
}

void GPUObject::Release()
{
}

void GPUObject::ClearDataLost()
{
    dataLost_ = false;
}

} // namespace dviglo
