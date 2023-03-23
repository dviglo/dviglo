// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "border_image.h"

namespace dviglo
{

/// %UI element that can be toggled between unchecked and checked state.
class DV_API CheckBox : public BorderImage
{
    DV_OBJECT(CheckBox, BorderImage);

public:
    /// Construct.
    explicit CheckBox();
    /// Destruct.
    ~CheckBox() override;
    /// Register object factory.
    static void register_object();

    /// Return UI rendering batches.
    void GetBatches(Vector<UIBatch>& batches, Vector<float>& vertexData, const IntRect& currentScissor) override;
    /// React to mouse click begin.
    void OnClickBegin
        (const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) override;
    /// React to a key press.
    void OnKey(Key key, MouseButtonFlags buttons, QualifierFlags qualifiers) override;

    /// Set checked state.
    void SetChecked(bool enable);
    /// Set checked image offset.
    void SetCheckedOffset(const IntVector2& offset);
    /// Set checked image offset.
    void SetCheckedOffset(int x, int y);

    /// Return whether is checked.
    bool IsChecked() const { return checked_; }

    /// Return checked image offset.
    const IntVector2& GetCheckedOffset() const { return checkedOffset_; }

protected:
    /// Checked image offset.
    IntVector2 checkedOffset_;
    /// Current checked state.
    bool checked_;
};

}
