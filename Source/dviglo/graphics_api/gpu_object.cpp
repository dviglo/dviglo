// Copyright (c) 2008-2022 the Urho3D project
// Copyright (c) 2022-2022 the Dviglo project
// License: MIT

#include "../precompiled.h"

#include "../graphics_api/gpu_object.h"
#include "../graphics/graphics.h"

#include "../debug_new.h"

namespace Urho3D
{

GPUObject::GPUObject(Graphics* graphics) :
    graphics_(graphics)
{
    if (Graphics::GetGAPI() == GAPI_OPENGL)
        object_.name_ = 0;
    else
        object_.ptr_ = nullptr;

    if (graphics_)
        graphics->AddGPUObject(this);
}

GPUObject::~GPUObject()
{
    if (graphics_)
        graphics_->RemoveGPUObject(this);
}

void GPUObject::OnDeviceLost()
{
    if (Graphics::GetGAPI() == GAPI_OPENGL)
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

Graphics* GPUObject::GetGraphics() const
{
    return graphics_;
}

}
