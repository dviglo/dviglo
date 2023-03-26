// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../Sample.h"

namespace dviglo
{

class Button;
class LineEdit;
class Text;
class UiElement;

}

const int SERVER_PORT = 54654;

/// Chat example
/// This sample demonstrates:
///     - Starting up a network server or connecting to it
///     - Implementing simple chat functionality with network messages
class LANDiscovery : public Sample
{
    DV_OBJECT(LANDiscovery, Sample);

public:
    /// Construct.
    explicit LANDiscovery();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Create the UI.
    void create_ui();
    /// Subscribe to log message, UI and network events.
    void subscribe_to_events();
    /// Create a button to the button container.
    Button* CreateButton(const String& text, int width, IntVector2 position);
    /// Create label
    Text* CreateLabel(const String& text, IntVector2 pos);
    
    /// Handle found LAN server
    void HandleNetworkHostDiscovered(StringHash eventType, VariantMap& eventData);
    /// Start server
    void HandleStartServer(StringHash eventType, VariantMap& eventData);
    /// Stop server
    void HandleStopServer(StringHash eventType, VariantMap& eventData);
    /// Start network discovery
    void HandleDoNetworkDiscovery(StringHash eventType, VariantMap& eventData);
    /// Start server
    SharedPtr<Button> startServer_;
    /// Stop server
    SharedPtr<Button> stopServer_;
    /// Redo LAN discovery
    SharedPtr<Button> refreshServerList_;
    /// Found server list
    SharedPtr<Text> serverList_;
};
