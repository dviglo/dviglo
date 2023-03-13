// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/audio/audio.h>
#include <dviglo/audio/sound.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/input/input.h>
#include <dviglo/io/io_events.h>
#include <dviglo/io/log.h>
#include <dviglo/io/memory_buffer.h>
#include <dviglo/io/vector_buffer.h>
#include <dviglo/network/network.h>
#include <dviglo/network/network_events.h>
#include <dviglo/network/protocol.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/button.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/line_edit.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>
#include <dviglo/ui/ui_events.h>

#include "chat.h"

#include <dviglo/common/debug_new.h>

// Undefine Windows macro, as our Connection class has a function called SendMessage
#ifdef SendMessage
#undef SendMessage
#endif

// Identifier for the chat network messages
const int MSG_CHAT = MSG_USER + 0;
// UDP port we will use
const unsigned short CHAT_SERVER_PORT = 2345;

DV_DEFINE_APPLICATION_MAIN(Chat)

Chat::Chat()
{
}

void Chat::Start()
{
    // Execute base class startup
    Sample::Start();

    // Enable OS cursor
    DV_INPUT.SetMouseVisible(true);

    // Create the user interface
    CreateUI();

    // Subscribe to UI and network events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void Chat::CreateUI()
{
    SetLogoVisible(false); // We need the full rendering window

    UIElement* root = DV_UI.GetRoot();
    auto* uiStyle = DV_RES_CACHE.GetResource<XMLFile>("UI/DefaultStyle.xml");
    // Set style to the UI root so that elements will inherit it
    root->SetDefaultStyle(uiStyle);

    auto* font = DV_RES_CACHE.GetResource<Font>("Fonts/Anonymous Pro.ttf");
    chatHistoryText_ = root->CreateChild<Text>();
    chatHistoryText_->SetFont(font, 12);

    buttonContainer_ = root->CreateChild<UIElement>();
    buttonContainer_->SetFixedSize(DV_GRAPHICS.GetWidth(), 20);
    buttonContainer_->SetPosition(0, DV_GRAPHICS.GetHeight() - 20);
    buttonContainer_->SetLayoutMode(LM_HORIZONTAL);

    textEdit_ = buttonContainer_->CreateChild<LineEdit>();
    textEdit_->SetStyleAuto();

    sendButton_ = CreateButton("Send", 70);
    connectButton_ = CreateButton("Connect", 90);
    disconnectButton_ = CreateButton("Disconnect", 100);
    startServerButton_ = CreateButton("Start Server", 110);

    UpdateButtons();

    float rowHeight = chatHistoryText_->GetRowHeight();
    // Row height would be zero if the font failed to load
    if (rowHeight)
    {
        float numberOfRows = (DV_GRAPHICS.GetHeight() - 100) / rowHeight;
        chatHistory_.Resize(static_cast<unsigned int>(numberOfRows));
    }

    // No viewports or scene is defined. However, the default zone's fog color controls the fill color
    DV_RENDERER.GetDefaultZone()->SetFogColor(Color(0.0f, 0.0f, 0.1f));
}

void Chat::SubscribeToEvents()
{
    // Subscribe to UI element events
    SubscribeToEvent(textEdit_, E_TEXTFINISHED, DV_HANDLER(Chat, HandleSend));
    SubscribeToEvent(sendButton_, E_RELEASED, DV_HANDLER(Chat, HandleSend));
    SubscribeToEvent(connectButton_, E_RELEASED, DV_HANDLER(Chat, HandleConnect));
    SubscribeToEvent(disconnectButton_, E_RELEASED, DV_HANDLER(Chat, HandleDisconnect));
    SubscribeToEvent(startServerButton_, E_RELEASED, DV_HANDLER(Chat, HandleStartServer));

    // Subscribe to log messages so that we can pipe them to the chat window
    SubscribeToEvent(E_LOGMESSAGE, DV_HANDLER(Chat, HandleLogMessage));

    // Subscribe to network events
    SubscribeToEvent(E_NETWORKMESSAGE, DV_HANDLER(Chat, HandleNetworkMessage));
    SubscribeToEvent(E_SERVERCONNECTED, DV_HANDLER(Chat, HandleConnectionStatus));
    SubscribeToEvent(E_SERVERDISCONNECTED, DV_HANDLER(Chat, HandleConnectionStatus));
    SubscribeToEvent(E_CONNECTFAILED, DV_HANDLER(Chat, HandleConnectionStatus));
}

Button* Chat::CreateButton(const String& text, int width)
{
    auto* font = DV_RES_CACHE.GetResource<Font>("Fonts/Anonymous Pro.ttf");

    auto* button = buttonContainer_->CreateChild<Button>();
    button->SetStyleAuto();
    button->SetFixedWidth(width);

    auto* buttonText = button->CreateChild<Text>();
    buttonText->SetFont(font, 12);
    buttonText->SetAlignment(HA_CENTER, VA_CENTER);
    buttonText->SetText(text);

    return button;
}

void Chat::ShowChatText(const String& row)
{
    chatHistory_.Erase(0);
    chatHistory_.Push(row);

    // Concatenate all the rows in history
    String allRows;
    for (const String& row : chatHistory_)
        allRows += row + "\n";

    chatHistoryText_->SetText(allRows);
}

void Chat::UpdateButtons()
{
    Connection* serverConnection = DV_NET.GetServerConnection();
    bool serverRunning = DV_NET.IsServerRunning();

    // Show and hide buttons so that eg. Connect and Disconnect are never shown at the same time
    sendButton_->SetVisible(serverConnection != nullptr);
    connectButton_->SetVisible(!serverConnection && !serverRunning);
    disconnectButton_->SetVisible(serverConnection || serverRunning);
    startServerButton_->SetVisible(!serverConnection && !serverRunning);
}

void Chat::HandleLogMessage(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace LogMessage;

    ShowChatText(eventData[P_MESSAGE].GetString());
}

void Chat::HandleSend(StringHash /*eventType*/, VariantMap& eventData)
{
    String text = textEdit_->GetText();
    if (text.Empty())
        return; // Do not send an empty message

    Connection* serverConnection = DV_NET.GetServerConnection();

    if (serverConnection)
    {
        // A VectorBuffer object is convenient for constructing a message to send
        VectorBuffer msg;
        msg.WriteString(text);
        // Send the chat message as in-order and reliable
        serverConnection->SendMessage(MSG_CHAT, true, true, msg);
        // Empty the text edit after sending
        textEdit_->SetText(String::EMPTY);
    }
}

void Chat::HandleConnect(StringHash /*eventType*/, VariantMap& eventData)
{
    String address = textEdit_->GetText().Trimmed();
    if (address.Empty())
        address = "localhost"; // Use localhost to connect if nothing else specified
    // Empty the text edit after reading the address to connect to
    textEdit_->SetText(String::EMPTY);

    // Connect to server, do not specify a client scene as we are not using scene replication, just messages.
    // At connect time we could also send identity parameters (such as username) in a VariantMap, but in this
    // case we skip it for simplicity
    DV_NET.Connect(address, CHAT_SERVER_PORT, nullptr);

    UpdateButtons();
}

void Chat::HandleDisconnect(StringHash /*eventType*/, VariantMap& eventData)
{
    Connection* serverConnection = DV_NET.GetServerConnection();
    // If we were connected to server, disconnect
    if (serverConnection)
        serverConnection->Disconnect();
    // Or if we were running a server, stop it
    else if (DV_NET.IsServerRunning())
        DV_NET.StopServer();

    UpdateButtons();
}

void Chat::HandleStartServer(StringHash /*eventType*/, VariantMap& eventData)
{
    DV_NET.StartServer(CHAT_SERVER_PORT);

    UpdateButtons();
}

void Chat::HandleNetworkMessage(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace NetworkMessage;

    int msgID = eventData[P_MESSAGEID].GetI32();
    if (msgID == MSG_CHAT)
    {
        const Vector<byte>& data = eventData[P_DATA].GetBuffer();
        // Use a MemoryBuffer to read the message data so that there is no unnecessary copying
        MemoryBuffer msg(data);
        String text = msg.ReadString();

        // If we are the server, prepend the sender's IP address and port and echo to everyone
        // If we are a client, just display the message
        if (DV_NET.IsServerRunning())
        {
            auto* sender = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());

            text = sender->ToString() + " " + text;

            VectorBuffer sendMsg;
            sendMsg.WriteString(text);
            // Broadcast as in-order and reliable
            DV_NET.BroadcastMessage(MSG_CHAT, true, true, sendMsg);
        }

        ShowChatText(text);
    }
}

void Chat::HandleConnectionStatus(StringHash /*eventType*/, VariantMap& eventData)
{
    UpdateButtons();
}
