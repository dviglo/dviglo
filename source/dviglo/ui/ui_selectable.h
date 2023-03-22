// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../math/color.h"
#include "ui_element.h"

namespace dviglo
{

class DV_API UISelectable : public UiElement
{
public:
    DV_OBJECT(UISelectable, UiElement);

    /// Construct.
    using UiElement::UiElement;
    /// Destruct.
    ~UISelectable() override = default;

    /// Register object factory.
    static void RegisterObject();

    /// Return UI rendering batches.
    void GetBatches(Vector<UIBatch>& batches, Vector<float>& vertexData, const IntRect& currentScissor) override;

    /// Set selection background color. Color with 0 alpha (default) disables.
    void SetSelectionColor(const Color& color);
    /// Set hover background color. Color with 0 alpha (default) disables.
    void SetHoverColor(const Color& color);

    /// Return selection background color.
    const Color& GetSelectionColor() const { return selectionColor_; }
    /// Return hover background color.
    const Color& GetHoverColor() const { return hoverColor_; }

protected:
    /// Selection background color.
    Color selectionColor_{Color::TRANSPARENT_BLACK};
    /// Hover background color.
    Color hoverColor_{Color::TRANSPARENT_BLACK};
};

}
