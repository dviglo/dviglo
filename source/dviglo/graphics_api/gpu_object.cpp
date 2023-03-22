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
    if (!GParams::is_headless())
        DV_GRAPHICS.AddGPUObject(this);
}

GPUObject::~GPUObject()
{
    if (!GParams::is_headless() && !Graphics::is_destructed())
        DV_GRAPHICS.RemoveGPUObject(this);
}

void GPUObject::OnDeviceLost()
{
    // В OpenGL объект уже потерян, очищаем имя объекта
    gpu_object_name_ = 0;
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
