// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "tooltip.h"
#include "ui.h"

namespace dviglo
{

extern const char* UI_CATEGORY;

ToolTip::ToolTip() :
    delay_(0.0f),
    hovered_(false)
{
    SetVisible(false);
}

ToolTip::~ToolTip() = default;

void ToolTip::register_object()
{
    DV_CONTEXT->RegisterFactory<ToolTip>(UI_CATEGORY);

    DV_COPY_BASE_ATTRIBUTES(UiElement);
    DV_ACCESSOR_ATTRIBUTE("Delay", GetDelay, SetDelay, 0.0f, AM_FILE);
}

void ToolTip::Update(float timeStep)
{
    // Track the element we are parented to for hovering. When we display, we move ourself to the root element
    // to ensure displaying on top
    UiElement* root = GetRoot();
    if (!root)
        return;
    if (parent_ != root)
        target_ = parent_;

    // If target is removed while we are displaying, we have no choice but to destroy ourself
    if (target_.Expired())
    {
        Remove();
        return;
    }

    bool hovering = target_->IsHovering() && target_->IsVisibleEffective();
    if (!hovering)
    {
        for (auto it = altTargets_.Begin(); it != altTargets_.End();)
        {
            SharedPtr<UiElement> target = it->Lock();
            if (!target)
                it = altTargets_.Erase(it);
            else
            {
                hovering = target->IsHovering() && target->IsVisibleEffective();

                if (hovering)
                    break;
                else
                    ++it;
            }
        }
    }

    if (hovering)
    {
        float effectiveDelay = delay_ > 0.0f ? delay_ : DV_UI->GetDefaultToolTipDelay();

        if (!hovered_)
        {
            hovered_ = true;
            displayAt_.Reset();
        }
        else if (displayAt_.GetMSec(false) >= (unsigned)(effectiveDelay * 1000.0f) && parent_ == target_)
        {
            originalPosition_ = GetPosition();
            IntVector2 screenPosition = GetScreenPosition();
            SetParent(root);
            SetPosition(screenPosition);
            SetVisible(true);
            // BringToFront() is unreliable in this case as it takes into account only input-enabled elements.
            // Rather just force priority to max
            SetPriority(M_MAX_INT);
        }
    }
    else
    {
        Reset();
    }
}

void ToolTip::Reset()
{
    if (IsVisible() && parent_ == GetRoot())
    {
        SetParent(target_);
        SetPosition(originalPosition_);
        SetVisible(false);
    }
    hovered_ = false;
    displayAt_.Reset();
}

void ToolTip::AddAltTarget(UiElement* target)
{
    altTargets_.Push(WeakPtr<UiElement>(target));
}

void ToolTip::SetDelay(float delay)
{
    delay_ = delay;
}

}
