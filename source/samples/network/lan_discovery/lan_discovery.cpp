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
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/button.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/line_edit.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>
#include <dviglo/ui/ui_events.h>

#include "lan_discovery.h"

#include <dviglo/common/debug_new.h>

// Undefine Windows macro, as our Connection class has a function called SendMessage
#ifdef SendMessage
#undef SendMessage
#endif

DV_DEFINE_APPLICATION_MAIN(LANDiscovery)

LANDiscovery::LANDiscovery()
{
}

void LANDiscovery::Start()
{
    // Execute base class startup
    Sample::Start();

    // Enable OS cursor
    DV_INPUT.SetMouseVisible(true);

    // Create the user interface
    create_ui();

    // Subscribe to UI and network events
    subscribe_to_events();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void LANDiscovery::create_ui()
{
    SetLogoVisible(true); // We need the full rendering window

    UiElement* root = DV_UI->GetRoot();
    auto* uiStyle = DV_RES_CACHE.GetResource<XmlFile>("ui/default_style.xml");
    // Set style to the UI root so that elements will inherit it
    root->SetDefaultStyle(uiStyle);

    int marginTop = 20;
    CreateLabel("1. Start server", IntVector2(20, marginTop-20));
    startServer_ = CreateButton("Start server", 160, IntVector2(20, marginTop));
    stopServer_ = CreateButton("Stop server", 160, IntVector2(20, marginTop));
    stopServer_->SetVisible(false);

    // Create client connection related fields
    marginTop += 80;
    CreateLabel("2. Discover LAN servers", IntVector2(20, marginTop-20));
    refreshServerList_ = CreateButton("Search...", 160, IntVector2(20, marginTop));

    marginTop += 80;
    CreateLabel("Local servers:", IntVector2(20, marginTop - 20));
    serverList_ = CreateLabel("", IntVector2(20, marginTop));

    // No viewports or scene is defined. However, the default zone's fog color controls the fill color
    DV_RENDERER.GetDefaultZone()->SetFogColor(Color(0.0f, 0.0f, 0.1f));
}

void LANDiscovery::subscribe_to_events()
{
    subscribe_to_event(E_NETWORKHOSTDISCOVERED, DV_HANDLER(LANDiscovery, HandleNetworkHostDiscovered));

    subscribe_to_event(startServer_, "Released", DV_HANDLER(LANDiscovery, HandleStartServer));
    subscribe_to_event(stopServer_, "Released", DV_HANDLER(LANDiscovery, HandleStopServer));
    subscribe_to_event(refreshServerList_, "Released", DV_HANDLER(LANDiscovery, HandleDoNetworkDiscovery));
}

Button* LANDiscovery::CreateButton(const String& text, int width, IntVector2 position)
{
    auto* font = DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf");

    auto* button = DV_UI->GetRoot()->create_child<Button>();
    button->SetStyleAuto();
    button->SetFixedWidth(width);
    button->SetFixedHeight(30);
    button->SetPosition(position);

    auto* buttonText = button->create_child<Text>();
    buttonText->SetFont(font, 12);
    buttonText->SetAlignment(HA_CENTER, VA_CENTER);
    buttonText->SetText(text);

    return button;
}

Text* LANDiscovery::CreateLabel(const String& text, IntVector2 pos)
{
    // Create log element to view latest logs from the system
    auto* font = DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf");
    auto* label = DV_UI->GetRoot()->create_child<Text>();
    label->SetFont(font, 12);
    label->SetColor(Color(0.0f, 1.0f, 0.0f));
    label->SetPosition(pos);
    label->SetText(text);
    return label;
}

void LANDiscovery::HandleNetworkHostDiscovered(StringHash eventType, VariantMap& eventData)
{
    using namespace NetworkHostDiscovered;
    DV_LOGINFO("Server discovered!");
    String text = serverList_->GetText();
    VariantMap data = eventData[P_BEACON].GetVariantMap();
    text += "\n" + data["Name"].GetString() + "(" + String(data["Players"].GetI32()) + ")" + eventData[P_ADDRESS].GetString() + ":" + String(eventData[P_PORT].GetI32());
    serverList_->SetText(text);
}

void LANDiscovery::HandleStartServer(StringHash eventType, VariantMap& eventData)
{
    if (DV_NET.StartServer(SERVER_PORT))
    {
        VariantMap data;
        data["Name"] = "Test server";
        data["Players"] = 100;
        /// Set data which will be sent to all who requests LAN network discovery
        DV_NET.SetDiscoveryBeacon(data);
        startServer_->SetVisible(false);
        stopServer_->SetVisible(true);
    }
}

void LANDiscovery::HandleStopServer(StringHash eventType, VariantMap& eventData)
{
    DV_NET.StopServer();
    startServer_->SetVisible(true);
    stopServer_->SetVisible(false);
}

void LANDiscovery::HandleDoNetworkDiscovery(StringHash eventType, VariantMap& eventData)
{
    /// Pass in the port that should be checked
    DV_NET.DiscoverHosts(SERVER_PORT);
    serverList_->SetText("");
}
