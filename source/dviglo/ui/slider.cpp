// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../input/input_events.h"
#include "../io/log.h"
#include "slider.h"
#include "ui_events.h"

#include "../common/debug_new.h"

namespace dviglo
{

const char* orientations[] =
{
    "Horizontal",
    "Vertical",
    nullptr
};

extern const char* UI_CATEGORY;

Slider::Slider() :
    orientation_(O_HORIZONTAL),
    range_(1.0f),
    value_(0.0f),
    dragSlider_(false),
    repeatRate_(0.0f)
{
    SetEnabled(true);
    knob_ = create_child<BorderImage>("S_Knob");
    knob_->SetInternal(true);

    UpdateSlider();
}

Slider::~Slider() = default;

void Slider::register_object()
{
    DV_CONTEXT->RegisterFactory<Slider>(UI_CATEGORY);

    DV_COPY_BASE_ATTRIBUTES(BorderImage);
    DV_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Is Enabled", true);
    DV_ENUM_ACCESSOR_ATTRIBUTE("Orientation", GetOrientation, SetOrientation, orientations, O_HORIZONTAL, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Range", GetRange, SetRange, 1.0f, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Value", GetValue, SetValue, 0.0f, AM_FILE);
    DV_ACCESSOR_ATTRIBUTE("Repeat Rate", GetRepeatRate, SetRepeatRate, 0.0f, AM_FILE);
}

void Slider::Update(float timeStep)
{
    if (dragSlider_)
        hovering_ = true;

    // Propagate hover effect to the slider knob
    knob_->SetHovering(hovering_);
    knob_->SetSelected(hovering_);
}

void Slider::OnHover(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor)
{
    BorderImage::OnHover(position, screenPosition, buttons, qualifiers, cursor);

    // Show hover effect if inside the slider knob
    hovering_ = knob_->IsInside(screenPosition, true);

    // If not hovering on the knob, send it as page event
    if (!hovering_)
        Page(position, (bool)(buttons & MOUSEB_LEFT));
}

void Slider::OnClickBegin(const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers,
    Cursor* cursor)
{
    selected_ = true;
    hovering_ = knob_->IsInside(screenPosition, true);
    if (!hovering_ && button == MOUSEB_LEFT)
        Page(position, true);
}

void Slider::OnClickEnd(const IntVector2& position, const IntVector2& screenPosition, MouseButton button, MouseButtonFlags buttons, QualifierFlags qualifiers,
    Cursor* cursor, UiElement* beginElement)
{
    hovering_ = knob_->IsInside(screenPosition, true);
    if (!hovering_ && button == MOUSEB_LEFT)
        Page(position, false);
}

void Slider::OnDragBegin(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor)
{
    UiElement::OnDragBegin(position, screenPosition, buttons, qualifiers, cursor);

    if (buttons == MOUSEB_LEFT)
    {
        dragBeginCursor_ = position;
        dragBeginPosition_ = knob_->GetPosition();
        dragSlider_ = knob_->IsInside(screenPosition, true);
    }
}

void Slider::OnDragMove(const IntVector2& position, const IntVector2& screenPosition, const IntVector2& deltaPos, MouseButtonFlags buttons,
    QualifierFlags qualifiers, Cursor* cursor)
{
    if (!editable_ || !dragSlider_ || GetSize() == knob_->GetSize())
        return;

    float newValue;
    IntVector2 delta = position - dragBeginCursor_;

    if (orientation_ == O_HORIZONTAL)
    {
        int newX = Clamp(dragBeginPosition_.x + delta.x, 0, GetWidth() - knob_->GetWidth());
        knob_->SetPosition(newX, 0);
        newValue = (float)newX * range_ / (float)(GetWidth() - knob_->GetWidth());
    }
    else
    {
        int newY = Clamp(dragBeginPosition_.y + delta.y, 0, GetHeight() - knob_->GetHeight());
        knob_->SetPosition(0, newY);
        newValue = (float)newY * range_ / (float)(GetHeight() - knob_->GetHeight());
    }

    SetValue(newValue);
}

void Slider::OnDragEnd(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags dragButtons, MouseButtonFlags releaseButtons, Cursor* cursor)
{
    UiElement::OnDragEnd(position, screenPosition, dragButtons, releaseButtons, cursor);

    if (dragButtons == MOUSEB_LEFT)
    {
        dragSlider_ = false;
        selected_ = false;
    }
}

void Slider::OnResize(const IntVector2& newSize, const IntVector2& delta)
{
    UpdateSlider();
}

void Slider::SetOrientation(Orientation orientation)
{
    orientation_ = orientation;
    UpdateSlider();
}

void Slider::SetRange(float range)
{
    range = Max(range, 0.0f);
    if (range != range_)
    {
        range_ = range;
        UpdateSlider();
    }
}

void Slider::SetValue(float value)
{
    value = Clamp(value, 0.0f, range_);
    if (value != value_)
    {
        value_ = value;
        UpdateSlider();

        using namespace SliderChanged;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_ELEMENT] = this;
        eventData[P_VALUE] = value_;
        SendEvent(E_SLIDERCHANGED, eventData);
    }
}

void Slider::ChangeValue(float delta)
{
    SetValue(value_ + delta);
}

void Slider::SetRepeatRate(float rate)
{
    repeatRate_ = Max(rate, 0.0f);
}

bool Slider::FilterImplicitAttributes(XmlElement& dest) const
{
    if (!BorderImage::FilterImplicitAttributes(dest))
        return false;

    XmlElement childElem = dest.GetChild("element");
    if (!childElem)
        return false;
    if (!RemoveChildXML(childElem, "Name", "S_Knob"))
        return false;
    if (!RemoveChildXML(childElem, "Position"))
        return false;
    if (!RemoveChildXML(childElem, "Size"))
        return false;

    return true;
}

void Slider::UpdateSlider()
{
    const IntRect& border = knob_->GetBorder();

    if (range_ > 0.0f)
    {
        if (orientation_ == O_HORIZONTAL)
        {
            int sliderLength = (int)Max((float)GetWidth() / (range_ + 1.0f), (float)(border.left_ + border.right_));

            if (knob_->IsFixedWidth())
                sliderLength = knob_->GetWidth();

            float sliderPos = (float)(GetWidth() - sliderLength) * value_ / range_;

            if (!knob_->IsFixedSize())
            {
                knob_->SetSize(sliderLength, GetHeight());
                knob_->SetPosition(Clamp(RoundToInt(sliderPos), 0, GetWidth() - knob_->GetWidth()), 0);
            }
            else
                knob_->SetPosition(Clamp((int)(sliderPos), 0, GetWidth() - knob_->GetWidth()), 0);
        }
        else
        {
            int sliderLength = (int)Max((float)GetHeight() / (range_ + 1.0f), (float)(border.top_ + border.bottom_));

            if (knob_->IsFixedHeight())
                sliderLength = knob_->GetHeight();

            float sliderPos = (float)(GetHeight() - sliderLength) * value_ / range_;

            if (!knob_->IsFixedSize())
            {
                knob_->SetSize(GetWidth(), sliderLength);
                knob_->SetPosition(0, Clamp(RoundToInt(sliderPos), 0, GetHeight() - knob_->GetHeight()));
            }
            else
                knob_->SetPosition(0, Clamp(RoundToInt(sliderPos), 0, GetHeight() - knob_->GetHeight()));
        }
    }
    else
    {
        if (!knob_->IsFixedSize())
            knob_->SetSize(GetSize());

        knob_->SetPosition(0, 0);
    }
}

void Slider::Page(const IntVector2& position, bool pressed)
{
    if (!editable_)
        return;

    IntVector2 offsetXY = position - knob_->GetPosition() - knob_->GetSize() / 2;
    int offset = orientation_ == O_HORIZONTAL ? offsetXY.x : offsetXY.y;
    float length = (float)(orientation_ == O_HORIZONTAL ? GetWidth() : GetHeight());

    using namespace SliderPaged;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_ELEMENT] = this;
    eventData[P_OFFSET] = offset;

    // Start transmitting repeated pages after the initial press
    if (selected_ && pressed && repeatRate_ > 0.0f &&
        repeatTimer_.GetMSec(false) >= Lerp(1000.0f / repeatRate_, 0.0f, Abs(offset) / length))
        repeatTimer_.Reset();
    else
        pressed = false;

    eventData[P_PRESSED] = pressed;

    SendEvent(E_SLIDERPAGED, eventData);
}

}
