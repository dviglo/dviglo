// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "gpu_object.h"
#include "../graphics/graphics.h"

#include "../common/debug_new.h"

namespace dviglo
{

GpuObject::GpuObject()
{
    if (!GParams::is_headless())
        DV_GRAPHICS->AddGPUObject(this);
}

GpuObject::~GpuObject()
{
    if (DV_GRAPHICS) // Подсистема может быть уже уничтожена
        DV_GRAPHICS->RemoveGPUObject(this);
}

void GpuObject::OnDeviceLost()
{
    // В OpenGL объект уже потерян, очищаем имя объекта
    gpu_object_name_ = 0;
}

void GpuObject::OnDeviceReset()
{
}

void GpuObject::Release()
{
}

void GpuObject::ClearDataLost()
{
    dataLost_ = false;
}

} // namespace dviglo
