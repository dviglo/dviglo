// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../navigation/navigable.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* NAVIGATION_CATEGORY;

Navigable::Navigable(Context* context) :
    Component(context),
    recursive_(true)
{
}

Navigable::~Navigable() = default;

void Navigable::RegisterObject(Context* context)
{
    context->RegisterFactory<Navigable>(NAVIGATION_CATEGORY);

    DV_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, true, AM_DEFAULT);
    DV_ATTRIBUTE("Recursive", recursive_, true, AM_DEFAULT);
}

void Navigable::SetRecursive(bool enable)
{
    recursive_ = enable;
}

}
