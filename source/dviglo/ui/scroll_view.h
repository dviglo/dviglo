// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "ui_element.h"

namespace dviglo
{

class BorderImage;
class ScrollBar;

/// Scrollable %UI element for showing a (possibly large) child element.
class DV_API ScrollView : public UiElement
{
    DV_OBJECT(ScrollView, UiElement);

public:
    /// Construct.
    explicit ScrollView();
    /// Destruct.
    ~ScrollView() override;
    /// Register object factory.
    static void RegisterObject();

    /// Apply attribute changes that can not be applied immediately.
    void ApplyAttributes() override;
    /// React to mouse wheel.
    void OnWheel(int delta, MouseButtonFlags buttons, QualifierFlags qualifiers) override;
    /// React to a key press.
    void OnKey(Key key, MouseButtonFlags buttons, QualifierFlags qualifiers) override;
    /// React to resize.
    void OnResize(const IntVector2& newSize, const IntVector2& delta) override;

    /// Return whether the element could handle wheel input.
    bool IsWheelHandler() const override { return true; }

    /// Set content element.
    void SetContentElement(UiElement* element);
    /// Set view offset from the top-left corner.
    void SetViewPosition(const IntVector2& position);
    /// Set view offset from the top-left corner.
    void SetViewPosition(int x, int y);
    /// Set scrollbars' visibility manually. Disables scrollbar autoshow/hide.
    void SetScrollBarsVisible(bool horizontal, bool vertical);
    /// Set horizontal scrollbar visibility manually. Disables scrollbar autoshow/hide.
    void SetHorizontalScrollBarVisible(bool visible);
    /// Set vertical scrollbar visibility manually. Disables scrollbar autoshow/hide.
    void SetVerticalScrollBarVisible(bool visible);
    /// Set whether to automatically show/hide scrollbars. Default true.
    void SetScrollBarsAutoVisible(bool enable);
    /// Set arrow key scroll step. Also sets it on the scrollbars.
    void SetScrollStep(float step);
    /// Set arrow key page step.
    void SetPageStep(float step);

    /// Return view offset from the top-left corner.
    const IntVector2& GetViewPosition() const { return viewPosition_; }

    /// Return content element.
    UiElement* GetContentElement() const { return contentElement_; }

    /// Return horizontal scroll bar.
    ScrollBar* GetHorizontalScrollBar() const { return horizontalScrollBar_; }

    /// Return vertical scroll bar.
    ScrollBar* GetVerticalScrollBar() const { return verticalScrollBar_; }

    /// Return scroll panel.
    BorderImage* GetScrollPanel() const { return scrollPanel_; }

    /// Return whether scrollbars are automatically shown/hidden.
    bool GetScrollBarsAutoVisible() const { return scrollBarsAutoVisible_; }

    /// Return whether the horizontal scrollbar is visible.
    bool GetHorizontalScrollBarVisible() const;

    /// Return whether the vertical scrollbar is visible.
    bool GetVerticalScrollBarVisible() const;

    /// Return arrow key scroll step.
    float GetScrollStep() const;

    /// Return arrow key page step.
    float GetPageStep() const { return pageStep_; }

    /// Set view position attribute.
    void SetViewPositionAttr(const IntVector2& value);

protected:
    /// Filter implicit attributes in serialization process.
    bool FilterImplicitAttributes(XMLElement& dest) const override;
    /// Filter implicit attributes in serialization process for internal scroll bar.
    bool FilterScrollBarImplicitAttributes(XMLElement& dest, const String& name) const;
    /// Resize panel based on scrollbar visibility.
    void UpdatePanelSize();
    /// Recalculate view size, validate view position and update scrollbars.
    void UpdateViewSize();
    /// Update the scrollbars' ranges and positions.
    void UpdateScrollBars();
    /// Limit and update the view with a new position.
    void UpdateView(const IntVector2& position);

    /// Content element.
    SharedPtr<UiElement> contentElement_;
    /// Horizontal scroll bar.
    SharedPtr<ScrollBar> horizontalScrollBar_;
    /// Vertical scroll bar.
    SharedPtr<ScrollBar> verticalScrollBar_;
    /// Scroll panel element.
    SharedPtr<BorderImage> scrollPanel_;
    /// Current view offset from the top-left corner.
    IntVector2 viewPosition_;
    /// Total view size.
    IntVector2 viewSize_;
    /// View offset attribute.
    IntVector2 viewPositionAttr_;
    /// Arrow key page step.
    float pageStep_;
    /// Automatically show/hide scrollbars flag.
    bool scrollBarsAutoVisible_;
    /// Ignore scrollbar events flag. Used to prevent possible endless loop when resizing.
    bool ignoreEvents_;
    /// Resize content widget width to match panel. Internal flag, used by the ListView class.
    bool resizeContentWidth_;

private:
    /// Handle scrollbar value changed.
    void HandleScrollBarChanged(StringHash eventType, VariantMap& eventData);
    /// Handle scrollbar visibility changed.
    void HandleScrollBarVisibleChanged(StringHash eventType, VariantMap& eventData);
    /// Handle content element resized.
    void HandleElementResized(StringHash eventType, VariantMap& eventData);
};

}
