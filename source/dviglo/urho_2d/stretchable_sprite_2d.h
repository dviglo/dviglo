// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "static_sprite_2d.h"

namespace dviglo
{
/// Stretchable sprite component.
class DV_API StretchableSprite2D : public StaticSprite2D
{
    DV_OBJECT(StretchableSprite2D, StaticSprite2D);

public:
    /// Construct.
    explicit StretchableSprite2D();
    /// Register object factory. Drawable2D must be registered first.
    static void RegisterObject();

    /// Set border as number of pixels from each side.
    void SetBorder(const IntRect& border);
    /// Get border as number of pixels from each side.
    const IntRect& GetBorder() const { return border_; }

protected:
    /// Update source batches.
    void UpdateSourceBatches() override;

    /// The border, represented by the number of pixels from each side.
    IntRect border_; // absolute border in pixels
};

}
