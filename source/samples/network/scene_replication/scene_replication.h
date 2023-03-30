// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../sample.h"

namespace dviglo
{

class Button;
class Connection;
class Scene;
class Text;
class UiElement;

}

/// Scene network replication example.
/// This sample demonstrates:
///     - Creating a scene in which network clients can join
///     - Giving each client an object to control and sending the controls from the clients to the server
///       where the authoritative simulation happens
///     - Controlling a physics object's movement by applying forces
class SceneReplication : public Sample
{
    DV_OBJECT(SceneReplication, Sample);

public:
    /// Construct.
    explicit SceneReplication();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void create_scene();
    /// Construct instruction text and the login / start server UI.
    void create_ui();
    /// Set up viewport.
    void setup_viewport();
    /// Subscribe to update, UI and network events.
    void subscribe_to_events();
    /// Create a button to the button container.
    Button* CreateButton(const String& text, int width);
    /// Update visibility of buttons according to connection and server status.
    void UpdateButtons();
    /// Create a controllable ball object and return its scene node.
    Node* CreateControllableObject();
    /// Read input and move the camera.
    void move_camera();
    /// Handle the physics world pre-step event.
    void HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData);
    /// Handle the logic post-update event.
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle pressing the connect button.
    void HandleConnect(StringHash eventType, VariantMap& eventData);
    /// Handle pressing the disconnect button.
    void HandleDisconnect(StringHash eventType, VariantMap& eventData);
    /// Handle pressing the start server button.
    void HandleStartServer(StringHash eventType, VariantMap& eventData);
    /// Handle connection status change (just update the buttons that should be shown.)
    void HandleConnectionStatus(StringHash eventType, VariantMap& eventData);
    /// Handle a client connecting to the server.
    void HandleClientConnected(StringHash eventType, VariantMap& eventData);
    /// Handle a client disconnecting from the server.
    void HandleClientDisconnected(StringHash eventType, VariantMap& eventData);
    /// Handle remote event from server which tells our controlled object node ID.
    void HandleClientObjectID(StringHash eventType, VariantMap& eventData);

    /// Mapping from client connections to controllable objects.
    HashMap<Connection*, WeakPtr<Node>> serverObjects_;
    /// Button container element.
    SharedPtr<UiElement> buttonContainer_;
    /// Server address line editor element.
    SharedPtr<LineEdit> textEdit_;
    /// Connect button.
    SharedPtr<Button> connectButton_;
    /// Disconnect button.
    SharedPtr<Button> disconnectButton_;
    /// Start server button.
    SharedPtr<Button> startServerButton_;
    /// Instructions text.
    SharedPtr<Text> instructionsText_;
    /// ID of own controllable object (client only.)
    unsigned clientObjectID_{};
    /// Packets in per second
    SharedPtr<Text> packetsIn_;
    /// Packets out per second
    SharedPtr<Text> packetsOut_;
    /// Packet counter UI update timer
    Timer packetCounterTimer_;
};
