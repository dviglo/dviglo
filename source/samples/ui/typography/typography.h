// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../Sample.h"

/// Text rendering example.
/// Displays text at various sizes, with checkboxes to change the rendering parameters.
class Typography : public Sample
{
    DV_OBJECT(Typography, Sample);

public:
    /// Construct.
    explicit Typography();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    SharedPtr<UiElement> uielement_;

    void CreateText();
    SharedPtr<CheckBox> CreateCheckbox(const String& label, EventHandler* handler);
    SharedPtr<DropDownList> CreateMenu(const String& label, const char** items, EventHandler* handler);

    void HandleWhiteBackground(StringHash eventType, VariantMap& eventData);
    void HandleSRGB(StringHash eventType, VariantMap& eventData);
    void HandleForceAutoHint(StringHash eventType, VariantMap& eventData);
    void HandleFontHintLevel(StringHash eventType, VariantMap& eventData);
    void HandleFontSubpixel(StringHash eventType, VariantMap& eventData);
    void HandleFontOversampling(StringHash eventType, VariantMap& eventData);
};
