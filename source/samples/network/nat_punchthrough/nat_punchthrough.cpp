// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/audio/audio.h>
#include <dviglo/audio/sound.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/input/input.h>
#include <dviglo/io/log.h>
#include <dviglo/io/memory_buffer.h>
#include <dviglo/io/vector_buffer.h>
#include <dviglo/network/network.h>
#include <dviglo/network/network_events.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/ui/button.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/line_edit.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

#include "nat_punchthrough.h"

// Undefine Windows macro, as our Connection class has a function called SendMessage
#ifdef SendMessage
#undef SendMessage
#endif

DV_DEFINE_APPLICATION_MAIN(NATPunchtrough)

NATPunchtrough::NATPunchtrough()
{
}

void NATPunchtrough::Start()
{
    // Execute base class startup
    Sample::Start();

    // Enable OS cursor
    GetSubsystem<Input>()->SetMouseVisible(true);

    // Create the user interface
    CreateUI();

    // Subscribe to UI and network events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void NATPunchtrough::CreateUI()
{
    SetLogoVisible(true); // We need the full rendering window

    UIElement* root = GetSubsystem<UI>()->GetRoot();
    auto* uiStyle = DV_RES_CACHE.GetResource<XMLFile>("UI/DefaultStyle.xml");
    // Set style to the UI root so that elements will inherit it
    root->SetDefaultStyle(uiStyle);

    auto* font = DV_RES_CACHE.GetResource<Font>("Fonts/Anonymous Pro.ttf");
    logHistoryText_ = root->CreateChild<Text>();
    logHistoryText_->SetFont(font, 12);
    logHistoryText_->SetPosition(20, -20);
    logHistoryText_->SetVerticalAlignment(VA_BOTTOM);
    logHistory_.Resize(20);

    // Create NAT server config fields
    int marginTop = 40;
    CreateLabel("1. Run NAT server somewhere, enter NAT server info and press 'Save NAT settings'", IntVector2(20, marginTop-20));
    natServerAddress_ = CreateLineEdit("127.0.0.1", 200, IntVector2(20, marginTop));
    natServerPort_ = CreateLineEdit("61111", 100, IntVector2(240, marginTop));
    saveNatSettingsButton_ = CreateButton("Save NAT settings", 160, IntVector2(360, marginTop));

    // Create server start button
    marginTop = 120;
    CreateLabel("2. Create server and give others your server GUID", IntVector2(20, marginTop-20));
    guid_ = CreateLineEdit("Your server GUID", 200, IntVector2(20, marginTop));
    startServerButton_ = CreateButton("Start server", 160, IntVector2(240, marginTop));

    // Create client connection related fields
    marginTop = 200;
    CreateLabel("3. Input local or remote server GUID", IntVector2(20, marginTop-20));
    serverGuid_ = CreateLineEdit("Remote server GUID", 200, IntVector2(20, marginTop));
    connectButton_ = CreateButton("Connect", 160, IntVector2(240, marginTop));

    // No viewports or scene is defined. However, the default zone's fog color controls the fill color
    GetSubsystem<Renderer>()->GetDefaultZone()->SetFogColor(Color(0.0f, 0.0f, 0.1f));
}

void NATPunchtrough::SubscribeToEvents()
{
    SubscribeToEvent(E_SERVERCONNECTED, DV_HANDLER(NATPunchtrough, HandleServerConnected));
    SubscribeToEvent(E_SERVERDISCONNECTED, DV_HANDLER(NATPunchtrough, HandleServerDisconnected));
    SubscribeToEvent(E_CONNECTFAILED, DV_HANDLER(NATPunchtrough, HandleConnectFailed));

    // NAT server connection related events
    SubscribeToEvent(E_NATMASTERCONNECTIONFAILED, DV_HANDLER(NATPunchtrough, HandleNatConnectionFailed));
    SubscribeToEvent(E_NATMASTERCONNECTIONSUCCEEDED, DV_HANDLER(NATPunchtrough, HandleNatConnectionSucceeded));
    SubscribeToEvent(E_NATMASTERDISCONNECTED, DV_HANDLER(NATPunchtrough, HandleNatDisconnected));

    // NAT punchtrough request events
    SubscribeToEvent(E_NETWORKNATPUNCHTROUGHSUCCEEDED, DV_HANDLER(NATPunchtrough, HandleNatPunchtroughSucceeded));
    SubscribeToEvent(E_NETWORKNATPUNCHTROUGHFAILED, DV_HANDLER(NATPunchtrough, HandleNatPunchtroughFailed));

    SubscribeToEvent(E_CLIENTCONNECTED, DV_HANDLER(NATPunchtrough, HandleClientConnected));
    SubscribeToEvent(E_CLIENTDISCONNECTED, DV_HANDLER(NATPunchtrough, HandleClientDisconnected));

    SubscribeToEvent(saveNatSettingsButton_, "Released", DV_HANDLER(NATPunchtrough, HandleSaveNatSettings));
    SubscribeToEvent(startServerButton_, "Released", DV_HANDLER(NATPunchtrough, HandleStartServer));
    SubscribeToEvent(connectButton_, "Released", DV_HANDLER(NATPunchtrough, HandleConnect));
}

Button* NATPunchtrough::CreateButton(const String& text, int width, IntVector2 position)
{
    auto* font = DV_RES_CACHE.GetResource<Font>("Fonts/Anonymous Pro.ttf");

    auto* button = GetSubsystem<UI>()->GetRoot()->CreateChild<Button>();
    button->SetStyleAuto();
    button->SetFixedWidth(width);
    button->SetFixedHeight(30);
    button->SetPosition(position);

    auto* buttonText = button->CreateChild<Text>();
    buttonText->SetFont(font, 12);
    buttonText->SetAlignment(HA_CENTER, VA_CENTER);
    buttonText->SetText(text);

    return button;
}

LineEdit* NATPunchtrough::CreateLineEdit(const String& placeholder, int width, IntVector2 pos)
{
    auto* textEdit = GetSubsystem<UI>()->GetRoot()->CreateChild<LineEdit>("");
    textEdit->SetStyleAuto();
    textEdit->SetFixedWidth(width);
    textEdit->SetFixedHeight(30);
    textEdit->SetText(placeholder);
    textEdit->SetPosition(pos);
    return textEdit;
}

void NATPunchtrough::CreateLabel(const String& text, IntVector2 pos)
{
    // Create log element to view latest logs from the system
    auto* font = DV_RES_CACHE.GetResource<Font>("Fonts/Anonymous Pro.ttf");
    auto* label = GetSubsystem<UI>()->GetRoot()->CreateChild<Text>();
    label->SetFont(font, 12);
    label->SetColor(Color(0.0f, 1.0f, 0.0f));
    label->SetPosition(pos);
    label->SetText(text);
}

void NATPunchtrough::ShowLogMessage(const String& row)
{
    logHistory_.Erase(0);
    logHistory_.Push(row);

    // Concatenate all the rows in history
    String allRows;
    for (const String& row : logHistory_)
        allRows += row + "\n";

    logHistoryText_->SetText(allRows);
}

void NATPunchtrough::HandleSaveNatSettings(StringHash eventType, VariantMap& eventData)
{
    // Save NAT server configuration
    DV_NET.SetNATServerInfo(natServerAddress_->GetText(), ToI32(natServerPort_->GetText()));
    ShowLogMessage("Saving NAT settings: " + natServerAddress_->GetText() + ":" + natServerPort_->GetText());
}

void NATPunchtrough::HandleServerConnected(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("Client: Server connected!");
}

void NATPunchtrough::HandleServerDisconnected(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("Client: Server disconnected!");
}

void NATPunchtrough::HandleConnectFailed(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("Client: Connection failed!");
}

void NATPunchtrough::HandleNatDisconnected(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("Disconnected from NAT master server");
}

void NATPunchtrough::HandleStartServer(StringHash eventType, VariantMap& eventData)
{
    DV_NET.StartServer(SERVER_PORT);
    ShowLogMessage("Server: Server started on port: " + String(SERVER_PORT));

    // Connect to the NAT server
    DV_NET.StartNATClient();
    ShowLogMessage("Server: Starting NAT client for server...");

    // Output our assigned GUID which others will use to connect to our server
    guid_->SetText(DV_NET.GetGUID());
    serverGuid_->SetText(DV_NET.GetGUID());
}

void NATPunchtrough::HandleConnect(StringHash eventType, VariantMap& eventData)
{
    VariantMap userData;
    userData["Name"] = "Urho3D";

    // Attempt connecting to server using custom GUID, Scene = null as a second parameter and user identity is passed as third parameter
    DV_NET.AttemptNATPunchtrough(serverGuid_->GetText(), nullptr, userData);
    ShowLogMessage("Client: Attempting NAT punchtrough to guid: " + serverGuid_->GetText());
}

void NATPunchtrough::HandleNatConnectionFailed(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("Connection to NAT master server failed!");
}

void NATPunchtrough::HandleNatConnectionSucceeded(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("Connection to NAT master server succeeded!");
}

void NATPunchtrough::HandleNatPunchtroughSucceeded(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("NAT punchtrough succeeded!");
}

void NATPunchtrough::HandleNatPunchtroughFailed(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("NAT punchtrough failed!");
}

void NATPunchtrough::HandleClientConnected(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("Server: Client connected!");
}

void NATPunchtrough::HandleClientDisconnected(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("Server: Client disconnected!");
}
