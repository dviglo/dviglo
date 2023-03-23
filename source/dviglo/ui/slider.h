// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "border_image.h"

namespace dviglo
{

/// %Slider bar %UI element.
class DV_API Slider : public BorderImage
{
    DV_OBJECT(Slider, BorderImage);

public:
    /// Construct.
    explicit Slider();
    /// Destruct.
    ~Slider() override;
    /// Register object factory.
    static void register_object();

    /// Perform UI element update.
    void Update(float timeStep) override;
    /// React to mouse hover.
    void OnHover(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) override;
    /// React to mouse click begin.
    void OnClickBegin
        (const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) override;
    /// React to mouse click end.
    void OnClickEnd
        (const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor,
            UiElement* beginElement) override;
    /// React to mouse drag begin.
    void
        OnDragBegin(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) override;
    /// React to mouse drag motion.
    void OnDragMove
        (const IntVector2& position, const IntVector2& screenPosition, const IntVector2& deltaPos, MouseButtonFlags buttons, QualifierFlags qualifiers,
            Cursor* cursor) override;
    /// React to mouse drag end.
    void
        OnDragEnd(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags dragButtons, MouseButtonFlags releaseButtons, Cursor* cursor) override;
    /// React to resize.
    void OnResize(const IntVector2& newSize, const IntVector2& delta) override;

    /// Set orientation type.
    void SetOrientation(Orientation orientation);
    /// Set slider range maximum value (minimum value is always 0).
    void SetRange(float range);
    /// Set slider current value.
    void SetValue(float value);
    /// Change value by a delta.
    void ChangeValue(float delta);
    /// Set paging minimum repeat rate (number of events per second).
    void SetRepeatRate(float rate);

    /// Return orientation type.
    Orientation GetOrientation() const { return orientation_; }

    /// Return slider range.
    float GetRange() const { return range_; }

    /// Return slider current value.
    float GetValue() const { return value_; }

    /// Return knob element.
    BorderImage* GetKnob() const { return knob_; }

    /// Return paging minimum repeat rate (number of events per second).
    float GetRepeatRate() const { return repeatRate_; }

protected:
    /// Filter implicit attributes in serialization process.
    bool FilterImplicitAttributes(XmlElement& dest) const override;
    /// Update slider knob position & size.
    void UpdateSlider();
    /// Send slider page event.
    void Page(const IntVector2& position, bool pressed);

    /// Slider knob.
    SharedPtr<BorderImage> knob_;
    /// Orientation.
    Orientation orientation_;
    /// Slider range.
    float range_;
    /// Slider current value.
    float value_;
    /// Internal flag of whether the slider is being dragged.
    bool dragSlider_;
    /// Original mouse cursor position at drag begin.
    IntVector2 dragBeginCursor_;
    /// Original slider position at drag begin.
    IntVector2 dragBeginPosition_;
    /// Paging repeat rate.
    float repeatRate_;
    /// Paging minimum repeat timer.
    Timer repeatTimer_;
};

}
