// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "border_image.h"
#include "../math/vector2.h"
#include "text.h"

namespace dviglo
{

/// %ProgressBar bar %UI element.
class DV_API ProgressBar : public BorderImage
{
    DV_OBJECT(ProgressBar, BorderImage);

public:
    /// Construct.
    explicit ProgressBar(Context *context);

    /// Destruct.
    ~ProgressBar() override;

    /// Register object factory.
    static void RegisterObject(Context *context);

    /// React to resize.
    void OnResize(const IntVector2& newSize, const IntVector2& delta) override;

    /// Set orientation type.
    void SetOrientation(Orientation orientation);

    /// Set ProgressBar range maximum value (minimum value is always 0).
    void SetRange(float range);

    /// Set ProgressBar current value.
    void SetValue(float value);

    /// Change value by a delta.
    void ChangeValue(float delta);

    /// Return orientation type.
    Orientation GetOrientation() const { return orientation_; }

    /// Return ProgressBar range.
    float GetRange() const { return range_; }

    /// Return ProgressBar current value.
    float GetValue() const { return value_; }

    /// Return knob element.
    BorderImage *GetKnob() const { return knob_; }

    /// Sets the loading percent style.
    void SetLoadingPercentStyle(const String &style) { loadingPercentStyle_ = style; }

    /// Returns the loading percent style.
    const String& GetLoadingPercentStyle() const { return loadingPercentStyle_; }

    /// Sets the flag to display the percent text.
    void SetShowPercentText(bool enable);

    /// Returns the flag to display the percent text.
    bool GetShowPercentText() const { return showPercentText_; }

protected:
    /// Filter implicit attributes in serialization process.
    bool FilterImplicitAttributes(XMLElement &dest) const override;

    /// Update ProgressBar knob position & size.
    void UpdateProgressBar();

    /// ProgressBar knob.
    SharedPtr<BorderImage> knob_;
    /// ProgressBar text.
    SharedPtr<Text> loadingText_;
    /// Orientation.
    Orientation orientation_;
    /// ProgressBar text style.
    String loadingPercentStyle_;
    /// ProgressBar range.
    float range_;
    /// ProgressBar current value.
    float value_;
    /// Flag to show the percent text.
    bool showPercentText_;
};

}
