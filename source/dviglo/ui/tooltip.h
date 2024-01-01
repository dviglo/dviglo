// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "../graphics_api/graphics_defs.h"
#include "ui_element.h"

namespace dviglo
{

/// Tooltip %UI element.
class DV_API ToolTip : public UiElement
{
    DV_OBJECT(ToolTip);

public:
    /// Construct.
    explicit ToolTip();
    /// Destruct.
    ~ToolTip() override;
    /// Register object factory.
    static void register_object();

    /// Perform UI element update.
    void Update(float timeStep) override;

    /// Hide tooltip if visible and reset parentage.
    void Reset();

    /// Add an alternative hover target.
    void AddAltTarget(UiElement* target);

    /// Set the delay in seconds until the tooltip shows once hovering. Set zero to use the default from the UI subsystem.
    void SetDelay(float delay);

    /// Return the delay in seconds until the tooltip shows once hovering.
    float GetDelay() const { return delay_; }

private:
    /// The element that is being tracked for hovering. Normally the parent element.
    WeakPtr<UiElement> target_;
    /// Alternative targets. Primarily targets parent.
    Vector<WeakPtr<UiElement>> altTargets_;
    /// Delay from hover start to displaying the tooltip.
    float delay_;
    /// Hover countdown has started.
    bool hovered_;
    /// Point at which the tooltip was set visible.
    Timer displayAt_;
    /// Original offset position to the parent.
    IntVector2 originalPosition_;
};

}
