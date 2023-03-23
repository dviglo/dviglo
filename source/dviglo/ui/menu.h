// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "button.h"

namespace dviglo
{

/// %Menu %UI element that optionally shows a popup.
class DV_API Menu : public Button
{
    DV_OBJECT(Menu, Button);

public:
    /// Construct.
    explicit Menu();
    /// Destruct.
    ~Menu() override;
    /// Register object factory.
    static void register_object();

    using UiElement::load_xml;
    using UiElement::save_xml;

    /// Load from XML data with style. Return true if successful.
    bool load_xml(const XmlElement& source, XmlFile* styleFile) override;
    /// Save as XML data. Return true if successful.
    bool save_xml(XmlElement& dest) const override;

    /// Perform UI element update.
    void Update(float timeStep) override;
    /// React to mouse hover.
    void OnHover(const IntVector2& position, const IntVector2& screenPosition, MouseButtonFlags buttons, QualifierFlags qualifiers, Cursor* cursor) override;
    /// React to the popup being shown.
    virtual void OnShowPopup();

    /// React to the popup being hidden.
    virtual void OnHidePopup() { }

    /// Set popup element to show on selection.
    void SetPopup(UiElement* popup);
    /// Set popup element offset.
    void SetPopupOffset(const IntVector2& offset);
    /// Set popup element offset.
    void SetPopupOffset(int x, int y);
    /// Force the popup to show or hide.
    void ShowPopup(bool enable);
    /// Set accelerator key (set zero key code to disable).
    void SetAccelerator(int key, int qualifiers);

    /// Return popup element.
    UiElement* GetPopup() const { return popup_; }

    /// Return popup element offset.
    const IntVector2& GetPopupOffset() const { return popupOffset_; }

    /// Return whether popup is open.
    bool GetShowPopup() const { return showPopup_; }

    /// Return accelerator key code, 0 if disabled.
    int GetAcceleratorKey() const { return acceleratorKey_; }

    /// Return accelerator qualifiers.
    int GetAcceleratorQualifiers() const { return acceleratorQualifiers_; }

protected:
    /// Filter implicit attributes in serialization process.
    virtual bool FilterPopupImplicitAttributes(XmlElement& dest) const;
    /// Popup element.
    SharedPtr<UiElement> popup_;
    /// Popup element offset.
    IntVector2 popupOffset_;
    /// Show popup flag.
    bool showPopup_;
    /// Accelerator key code.
    int acceleratorKey_;
    /// Accelerator qualifiers.
    int acceleratorQualifiers_;

private:
    /// Handle press and release for selection and toggling popup visibility.
    void HandlePressedReleased(StringHash eventType, VariantMap& eventData);
    /// Handle global focus change to check for hiding the popup.
    void HandleFocusChanged(StringHash eventType, VariantMap& eventData);
    /// Handle keypress for checking accelerator.
    void HandleKeyDown(StringHash eventType, VariantMap& eventData);
    /// Auto popup flag.
    bool autoPopup_;
};

}
