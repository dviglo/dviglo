// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

namespace dviglo
{

class Button;
class Text;
class UiElement;
class XmlFile;

/// Message box dialog. Manages its lifetime automatically, so the application does not need to hold a reference to it, and shouldn't attempt to destroy it manually.
class DV_API MessageBox : public Object
{
    DV_OBJECT(MessageBox);

public:
    /// Construct. If layout file is not given, use the default message box layout. If style file is not given, use the default style file from root UI element.
    explicit MessageBox(const String& messageString = String::EMPTY, const String& titleString = String::EMPTY,
        XmlFile* layoutFile = nullptr, XmlFile* styleFile = nullptr);
    /// Destruct.
    ~MessageBox() override;
    /// Register object factory.
    static void register_object();

    /// Set title text. No-ops if there is no title text element.
    void SetTitle(const String& text);
    /// Set message text. No-ops if there is no message text element.
    void SetMessage(const String& text);

    /// Return title text. Return empty string if there is no title text element.
    const String& GetTitle() const;
    /// Return message text. Return empty string if there is no message text element.
    const String& GetMessage() const;

    /// Return dialog window.
    UiElement* GetWindow() const { return window_; }

private:
    /// Handle events that dismiss the message box.
    void HandleMessageAcknowledged(StringHash eventType, VariantMap& eventData);

    /// UI element containing the whole UI layout. Typically it is a Window element type.
    UiElement* window_;
    /// Title text element.
    Text* titleText_;
    /// Message text element.
    Text* messageText_;
    /// OK button element.
    Button* okButton_;
};

}
