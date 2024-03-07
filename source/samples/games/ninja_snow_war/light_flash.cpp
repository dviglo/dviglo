// Copyright (c) the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#include "light_flash.h"

void LightFlash::register_object()
{
    DV_CONTEXT->RegisterFactory<LightFlash>();
}

LightFlash::LightFlash()
{
    duration = 2.0f;
}

void LightFlash::FixedUpdate(float timeStep)
{
    Light* light = node_->GetComponent<Light>();
    light->SetBrightness(light->GetBrightness() * Max(1.0f - timeStep * 10.0f, 0.0f));

    // Call superclass to handle lifetime
    GameObject::FixedUpdate(timeStep);
}
