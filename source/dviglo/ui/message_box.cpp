// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../io/log.h"
#include "../resource/resource_cache.h"
#include "../resource/xml_file.h"
#include "button.h"
#include "message_box.h"
#include "text.h"
#include "ui.h"
#include "ui_events.h"
#include "window.h"

namespace dviglo
{

MessageBox::MessageBox(const String& messageString, const String& titleString, XmlFile* layoutFile,
    XmlFile* styleFile) :
    window_(nullptr),
    titleText_(nullptr),
    messageText_(nullptr),
    okButton_(nullptr)
{
    // If layout file is not given, use the default message box layout
    if (!layoutFile)
    {
        layoutFile = DV_RES_CACHE->GetResource<XmlFile>("ui/message_box.xml");
        if (!layoutFile)    // Error is already logged
            return;         // Note: windowless MessageBox should not be used!
    }

    UiElement* root = DV_UI->GetRoot();

    {
        SharedPtr<UiElement> holder = DV_UI->LoadLayout(layoutFile, styleFile);
        if (!holder)    // Error is already logged
            return;
        window_ = holder;
        root->AddChild(window_);    // Take ownership of the object before SharedPtr goes out of scope
    }

    // Set the title and message strings if they are given
    titleText_ = window_->GetChildDynamicCast<Text>("TitleText", true);
    if (titleText_ && !titleString.Empty())
        titleText_->SetText(titleString);
    messageText_ = window_->GetChildDynamicCast<Text>("MessageText", true);
    if (messageText_ && !messageString.Empty())
        messageText_->SetText(messageString);

    // Center window after the message is set
    auto* window = dynamic_cast<Window*>(window_);
    if (window)
    {
        const IntVector2& size = window->GetSize();
        window->SetPosition((root->GetWidth() - size.x) / 2, (root->GetHeight() - size.y) / 2);
        window->SetModal(true);
        subscribe_to_event(window, E_MODALCHANGED, DV_HANDLER(MessageBox, HandleMessageAcknowledged));
    }

    // Bind the buttons (if any in the loaded UI layout) to event handlers
    okButton_ = window_->GetChildDynamicCast<Button>("OkButton", true);
    if (okButton_)
    {
        DV_UI->SetFocusElement(okButton_);
        subscribe_to_event(okButton_, E_RELEASED, DV_HANDLER(MessageBox, HandleMessageAcknowledged));
    }
    auto* cancelButton = window_->GetChildDynamicCast<Button>("CancelButton", true);
    if (cancelButton)
        subscribe_to_event(cancelButton, E_RELEASED, DV_HANDLER(MessageBox, HandleMessageAcknowledged));
    auto* closeButton = window_->GetChildDynamicCast<Button>("CloseButton", true);
    if (closeButton)
        subscribe_to_event(closeButton, E_RELEASED, DV_HANDLER(MessageBox, HandleMessageAcknowledged));

    // Increase reference count to keep Self alive
    AddRef();
}

MessageBox::~MessageBox()
{
    // This would remove the UI-element regardless of whether it is parented to UI's root or UI's modal-root
    if (window_)
        window_->Remove();
}

void MessageBox::register_object()
{
    DV_CONTEXT.RegisterFactory<MessageBox>();
}

void MessageBox::SetTitle(const String& text)
{
    if (titleText_)
        titleText_->SetText(text);
}

void MessageBox::SetMessage(const String& text)
{
    if (messageText_)
        messageText_->SetText(text);
}

const String& MessageBox::GetTitle() const
{
    return titleText_ ? titleText_->GetText() : String::EMPTY;
}

const String& MessageBox::GetMessage() const
{
    return messageText_ ? messageText_->GetText() : String::EMPTY;
}

void MessageBox::HandleMessageAcknowledged(StringHash eventType, VariantMap& eventData)
{
    using namespace MessageACK;

    VariantMap& newEventData = GetEventDataMap();
    newEventData[P_OK] = eventData[Released::P_ELEMENT] == okButton_;
    SendEvent(E_MESSAGEACK, newEventData);

    // Self destruct
    ReleaseRef();
}

}
