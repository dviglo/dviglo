// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "ui_selectable.h"

namespace dviglo
{

extern const char* UI_CATEGORY;

void UISelectable::RegisterObject()
{
    DV_CONTEXT.RegisterFactory<UISelectable>(UI_CATEGORY);

    DV_COPY_BASE_ATTRIBUTES(UiElement);
    DV_ATTRIBUTE("Selection Color", selectionColor_, Color::TRANSPARENT_BLACK, AM_FILE);
    DV_ATTRIBUTE("Hover Color", hoverColor_, Color::TRANSPARENT_BLACK, AM_FILE);
}

void UISelectable::GetBatches(Vector<UIBatch>& batches, Vector<float>& vertexData, const IntRect & currentScissor)
{
    // Hovering and/or whole selection batch
    if ((hovering_ && hoverColor_.a_ > 0.0) || (selected_ && selectionColor_.a_ > 0.0f))
    {
        bool both = hovering_ && selected_ && hoverColor_.a_ > 0.0 && selectionColor_.a_ > 0.0f;
        UIBatch batch(this, BLEND_ALPHA, currentScissor, nullptr, &vertexData);
        batch.SetColor(both ? selectionColor_.Lerp(hoverColor_, 0.5f) :
            (selected_ && selectionColor_.a_ > 0.0f ? selectionColor_ : hoverColor_));
        batch.AddQuad(0.f, 0.f, (float)GetWidth(), (float)GetHeight(), 0, 0);
        UIBatch::AddOrMerge(batch, batches);
    }

    // Reset hovering for next frame
    hovering_ = false;
}

void UISelectable::SetSelectionColor(const Color& color)
{
    selectionColor_ = color;
}

void UISelectable::SetHoverColor(const Color& color)
{
    hoverColor_ = color;
}

}
