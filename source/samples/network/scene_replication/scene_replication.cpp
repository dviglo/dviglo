// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/light.h>
#include <dviglo/graphics/material.h>
#include <dviglo/graphics/model.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/static_model.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/input/controls.h>
#include <dviglo/input/input.h>
#include <dviglo/io/log.h>
#include <dviglo/network/connection.h>
#include <dviglo/network/network.h>
#include <dviglo/network/network_events.h>
#include <dviglo/physics/collision_shape.h>
#include <dviglo/physics/physics_events.h>
#include <dviglo/physics/physics_world.h>
#include <dviglo/physics/rigid_body.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/button.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/line_edit.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>
#include <dviglo/ui/ui_events.h>

#include "scene_replication.h"

#include <dviglo/common/debug_new.h>

// UDP port we will use
static const unsigned short SERVER_PORT = 2345;
// Identifier for our custom remote event we use to tell the client which object they control
static const StringHash E_CLIENTOBJECTID("ClientObjectID");
// Identifier for the node ID parameter in the event data
static const StringHash P_ID("ID");

// Control bits we define
static const unsigned CTRL_FORWARD = 1;
static const unsigned CTRL_BACK = 2;
static const unsigned CTRL_LEFT = 4;
static const unsigned CTRL_RIGHT = 8;

DV_DEFINE_APPLICATION_MAIN(SceneReplication)

SceneReplication::SceneReplication()
{
}

void SceneReplication::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    create_scene();

    // Create the UI content
    create_ui();

    // Setup the viewport for displaying the scene
    setup_viewport();

    // Hook up to necessary events
    subscribe_to_events();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void SceneReplication::create_scene()
{
    scene_ = new Scene();

    ResourceCache& cache = DV_RES_CACHE;

    // Create octree and physics world with default settings. Create them as local so that they are not needlessly replicated
    // when a client connects
    scene_->create_component<Octree>(LOCAL);
    scene_->create_component<PhysicsWorld>(LOCAL);

    // All static scene content and the camera are also created as local, so that they are unaffected by scene replication and are
    // not removed from the client upon connection. Create a Zone component first for ambient lighting & fog control.
    Node* zoneNode = scene_->create_child("Zone", LOCAL);
    auto* zone = zoneNode->create_component<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.1f, 0.1f, 0.1f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create a directional light without shadows
    Node* lightNode = scene_->create_child("DirectionalLight", LOCAL);
    lightNode->SetDirection(Vector3(0.5f, -1.0f, 0.5f));
    auto* light = lightNode->create_component<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetColor(Color(0.2f, 0.2f, 0.2f));
    light->SetSpecularIntensity(1.0f);

    // Create a "floor" consisting of several tiles. Make the tiles physical but leave small cracks between them
    for (int y = -20; y <= 20; ++y)
    {
        for (int x = -20; x <= 20; ++x)
        {
            Node* floorNode = scene_->create_child("FloorTile", LOCAL);
            floorNode->SetPosition(Vector3(x * 20.2f, -0.5f, y * 20.2f));
            floorNode->SetScale(Vector3(20.0f, 1.0f, 20.0f));
            auto* floorObject = floorNode->create_component<StaticModel>();
            floorObject->SetModel(cache.GetResource<Model>("models/box.mdl"));
            floorObject->SetMaterial(cache.GetResource<Material>("materials/stone.xml"));

            auto* body = floorNode->create_component<RigidBody>();
            body->SetFriction(1.0f);
            auto* shape = floorNode->create_component<CollisionShape>();
            shape->SetBox(Vector3::ONE);
        }
    }

    // Create the camera. Limit far clip distance to match the fog
    // The camera needs to be created into a local node so that each client can retain its own camera, that is unaffected by
    // network messages. Furthermore, because the client removes all replicated scene nodes when connecting to a server scene,
    // the screen would become blank if the camera node was replicated (as only the locally created camera is assigned to a
    // viewport in SetupViewports() below)
    cameraNode_ = scene_->create_child("Camera", LOCAL);
    auto* camera = cameraNode_->create_component<Camera>();
    camera->SetFarClip(300.0f);

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
}

void SceneReplication::create_ui()
{
    UI& ui = DV_UI;
    UiElement* root = ui.GetRoot();
    auto* uiStyle = DV_RES_CACHE.GetResource<XmlFile>("ui/default_style.xml");
    // Set style to the UI root so that elements will inherit it
    root->SetDefaultStyle(uiStyle);

    // Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will
    // control the camera, and when visible, it can interact with the login UI
    SharedPtr<Cursor> cursor(new Cursor());
    cursor->SetStyleAuto(uiStyle);
    ui.SetCursor(cursor);
    // Set starting position of the cursor at the rendering window center
    cursor->SetPosition(DV_GRAPHICS.GetWidth() / 2, DV_GRAPHICS.GetHeight() / 2);

    // Construct the instructions text element
    instructionsText_ = ui.GetRoot()->create_child<Text>();
    instructionsText_->SetText(
        "Use WASD keys to move and RMB to rotate view"
    );
    instructionsText_->SetFont(DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf"), 15);
    // Position the text relative to the screen center
    instructionsText_->SetHorizontalAlignment(HA_CENTER);
    instructionsText_->SetVerticalAlignment(VA_CENTER);
    instructionsText_->SetPosition(0, DV_GRAPHICS.GetHeight() / 4);
    // Hide until connected
    instructionsText_->SetVisible(false);

    packetsIn_ = ui.GetRoot()->create_child<Text>();
    packetsIn_->SetText("Packets in : 0");
    packetsIn_->SetFont(DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf"), 15);
    packetsIn_->SetHorizontalAlignment(HA_LEFT);
    packetsIn_->SetVerticalAlignment(VA_CENTER);
    packetsIn_->SetPosition(10, -10);

    packetsOut_ = ui.GetRoot()->create_child<Text>();
    packetsOut_->SetText("Packets out: 0");
    packetsOut_->SetFont(DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf"), 15);
    packetsOut_->SetHorizontalAlignment(HA_LEFT);
    packetsOut_->SetVerticalAlignment(VA_CENTER);
    packetsOut_->SetPosition(10, 10);

    buttonContainer_ = root->create_child<UiElement>();
    buttonContainer_->SetFixedSize(500, 20);
    buttonContainer_->SetPosition(20, 20);
    buttonContainer_->SetLayoutMode(LM_HORIZONTAL);

    textEdit_ = buttonContainer_->create_child<LineEdit>();
    textEdit_->SetStyleAuto();

    connectButton_ = CreateButton("Connect", 90);
    disconnectButton_ = CreateButton("Disconnect", 100);
    startServerButton_ = CreateButton("Start Server", 110);

    UpdateButtons();
}

void SceneReplication::setup_viewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    DV_RENDERER.SetViewport(0, viewport);
}

void SceneReplication::subscribe_to_events()
{
    // Subscribe to fixed timestep physics updates for setting or applying controls
    subscribe_to_event(E_PHYSICSPRESTEP, DV_HANDLER(SceneReplication, HandlePhysicsPreStep));

    // Subscribe HandlePostUpdate() method for processing update events. Subscribe to PostUpdate instead
    // of the usual Update so that physics simulation has already proceeded for the frame, and can
    // accurately follow the object with the camera
    subscribe_to_event(E_POSTUPDATE, DV_HANDLER(SceneReplication, HandlePostUpdate));

    // Subscribe to button actions
    subscribe_to_event(connectButton_, E_RELEASED, DV_HANDLER(SceneReplication, HandleConnect));
    subscribe_to_event(disconnectButton_, E_RELEASED, DV_HANDLER(SceneReplication, HandleDisconnect));
    subscribe_to_event(startServerButton_, E_RELEASED, DV_HANDLER(SceneReplication, HandleStartServer));

    // Subscribe to network events
    subscribe_to_event(E_SERVERCONNECTED, DV_HANDLER(SceneReplication, HandleConnectionStatus));
    subscribe_to_event(E_SERVERDISCONNECTED, DV_HANDLER(SceneReplication, HandleConnectionStatus));
    subscribe_to_event(E_CONNECTFAILED, DV_HANDLER(SceneReplication, HandleConnectionStatus));
    subscribe_to_event(E_CLIENTCONNECTED, DV_HANDLER(SceneReplication, HandleClientConnected));
    subscribe_to_event(E_CLIENTDISCONNECTED, DV_HANDLER(SceneReplication, HandleClientDisconnected));
    // This is a custom event, sent from the server to the client. It tells the node ID of the object the client should control
    subscribe_to_event(E_CLIENTOBJECTID, DV_HANDLER(SceneReplication, HandleClientObjectID));
    // Events sent between client & server (remote events) must be explicitly registered or else they are not allowed to be received
    DV_NET.RegisterRemoteEvent(E_CLIENTOBJECTID);
}

Button* SceneReplication::CreateButton(const String& text, int width)
{
    auto* font = DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf");

    auto* button = buttonContainer_->create_child<Button>();
    button->SetStyleAuto();
    button->SetFixedWidth(width);

    auto* buttonText = button->create_child<Text>();
    buttonText->SetFont(font, 12);
    buttonText->SetAlignment(HA_CENTER, VA_CENTER);
    buttonText->SetText(text);

    return button;
}

void SceneReplication::UpdateButtons()
{
    Connection* serverConnection = DV_NET.GetServerConnection();
    bool serverRunning = DV_NET.IsServerRunning();

    // Show and hide buttons so that eg. Connect and Disconnect are never shown at the same time
    connectButton_->SetVisible(!serverConnection && !serverRunning);
    disconnectButton_->SetVisible(serverConnection || serverRunning);
    startServerButton_->SetVisible(!serverConnection && !serverRunning);
    textEdit_->SetVisible(!serverConnection && !serverRunning);
}

Node* SceneReplication::CreateControllableObject()
{
    // Create the scene node & visual representation. This will be a replicated object
    Node* ballNode = scene_->create_child("Ball");
    ballNode->SetPosition(Vector3(Random(40.0f) - 20.0f, 5.0f, -Random(40.0f) + 20.0f));
    ballNode->SetScale(0.5f);
    auto* ballObject = ballNode->create_component<StaticModel>();
    ballObject->SetModel(DV_RES_CACHE.GetResource<Model>("models/sphere.mdl"));
    ballObject->SetMaterial(DV_RES_CACHE.GetResource<Material>("materials/StoneSmall.xml"));

    // Create the physics components
    auto* body = ballNode->create_component<RigidBody>();
    body->SetMass(1.0f);
    body->SetFriction(1.0f);
    // In addition to friction, use motion damping so that the ball can not accelerate limitlessly
    body->SetLinearDamping(0.5f);
    body->SetAngularDamping(0.5f);
    auto* shape = ballNode->create_component<CollisionShape>();
    shape->SetSphere(1.0f);

    // Create a random colored point light at the ball so that can see better where is going
    auto* light = ballNode->create_component<Light>();
    light->SetRange(3.0f);
    light->SetColor(
        Color(0.5f + ((unsigned)Rand() & 1u) * 0.5f, 0.5f + ((unsigned)Rand() & 1u) * 0.5f, 0.5f + ((unsigned)Rand() & 1u) * 0.5f));

    return ballNode;
}

void SceneReplication::move_camera()
{
    // Right mouse button controls mouse cursor visibility: hide when pressed
    DV_UI.GetCursor()->SetVisible(!DV_INPUT.GetMouseButtonDown(MOUSEB_RIGHT));

    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch and only move the camera
    // when the cursor is hidden
    if (!DV_UI.GetCursor()->IsVisible())
    {
        IntVector2 mouseMove = DV_INPUT.GetMouseMove();
        yaw_ += MOUSE_SENSITIVITY * mouseMove.x;
        pitch_ += MOUSE_SENSITIVITY * mouseMove.y;
        pitch_ = Clamp(pitch_, 1.0f, 90.0f);
    }

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

    // Only move the camera / show instructions if we have a controllable object
    bool showInstructions = false;
    if (clientObjectID_)
    {
        Node* ballNode = scene_->GetNode(clientObjectID_);
        if (ballNode)
        {
            const float CAMERA_DISTANCE = 5.0f;

            // Move camera some distance away from the ball
            cameraNode_->SetPosition(ballNode->GetPosition() + cameraNode_->GetRotation() * Vector3::BACK * CAMERA_DISTANCE);
            showInstructions = true;
        }
    }

    instructionsText_->SetVisible(showInstructions);
}

void SceneReplication::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    // We only rotate the camera according to mouse movement since last frame, so do not need the time step
    move_camera();

    if (packetCounterTimer_.GetMSec(false) > 1000 && DV_NET.GetServerConnection())
    {
        packetsIn_->SetText("Packets  in: " + String(DV_NET.GetServerConnection()->GetPacketsInPerSec()));
        packetsOut_->SetText("Packets out: " + String(DV_NET.GetServerConnection()->GetPacketsOutPerSec()));
        packetCounterTimer_.Reset();
    }

    if (packetCounterTimer_.GetMSec(false) > 1000 && DV_NET.GetClientConnections().Size())
    {
        int packetsIn = 0;
        int packetsOut = 0;
        auto connections = DV_NET.GetClientConnections();
        for (auto it = connections.Begin(); it != connections.End(); ++it)
        {
            packetsIn += (*it)->GetPacketsInPerSec();
            packetsOut += (*it)->GetPacketsOutPerSec();
        }
        packetsIn_->SetText("Packets  in: " + String(packetsIn));
        packetsOut_->SetText("Packets out: " + String(packetsOut));
        packetCounterTimer_.Reset();
    }
}

void SceneReplication::HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData)
{
    // This function is different on the client and server. The client collects controls (WASD controls + yaw angle)
    // and sets them to its server connection object, so that they will be sent to the server automatically at a
    // fixed rate, by default 30 FPS. The server will actually apply the controls (authoritative simulation.)
    Connection* serverConnection = DV_NET.GetServerConnection();

    // Client: collect controls
    if (serverConnection)
    {
        Controls controls;

        // Copy mouse yaw
        controls.yaw_ = yaw_;

        // Only apply WASD controls if there is no focused UI element
        if (!DV_UI.GetFocusElement())
        {
            controls.Set(CTRL_FORWARD, DV_INPUT.GetKeyDown(KEY_W));
            controls.Set(CTRL_BACK, DV_INPUT.GetKeyDown(KEY_S));
            controls.Set(CTRL_LEFT, DV_INPUT.GetKeyDown(KEY_A));
            controls.Set(CTRL_RIGHT, DV_INPUT.GetKeyDown(KEY_D));
        }

        serverConnection->SetControls(controls);
        // In case the server wants to do position-based interest management using the NetworkPriority components, we should also
        // tell it our observer (camera) position. In this sample it is not in use, but eg. the NinjaSnowWar game uses it
        serverConnection->SetPosition(cameraNode_->GetPosition());
    }
    // Server: apply controls to client objects
    else if (DV_NET.IsServerRunning())
    {
        const Vector<SharedPtr<Connection>>& connections = DV_NET.GetClientConnections();

        for (Connection* connection : connections)
        {
            // Get the object this connection is controlling
            Node* ballNode = serverObjects_[connection];
            if (!ballNode)
                continue;

            RigidBody* body = ballNode->GetComponent<RigidBody>();

            // Get the last controls sent by the client
            const Controls& controls = connection->GetControls();
            // Torque is relative to the forward vector
            Quaternion rotation(0.0f, controls.yaw_, 0.0f);

            const float MOVE_TORQUE = 3.0f;

            // Movement torque is applied before each simulation step, which happen at 60 FPS. This makes the simulation
            // independent from rendering framerate. We could also apply forces (which would enable in-air control),
            // but want to emphasize that it's a ball which should only control its motion by rolling along the ground
            if (controls.buttons_ & CTRL_FORWARD)
                body->ApplyTorque(rotation * Vector3::RIGHT * MOVE_TORQUE);
            if (controls.buttons_ & CTRL_BACK)
                body->ApplyTorque(rotation * Vector3::LEFT * MOVE_TORQUE);
            if (controls.buttons_ & CTRL_LEFT)
                body->ApplyTorque(rotation * Vector3::FORWARD * MOVE_TORQUE);
            if (controls.buttons_ & CTRL_RIGHT)
                body->ApplyTorque(rotation * Vector3::BACK * MOVE_TORQUE);
        }
    }
}

void SceneReplication::HandleConnect(StringHash eventType, VariantMap& eventData)
{
    String address = textEdit_->GetText().Trimmed();
    if (address.Empty())
        address = "localhost"; // Use localhost to connect if nothing else specified

    // Connect to server, specify scene to use as a client for replication
    clientObjectID_ = 0; // Reset own object ID from possible previous connection
    DV_NET.Connect(address, SERVER_PORT, scene_);

    UpdateButtons();
}

void SceneReplication::HandleDisconnect(StringHash eventType, VariantMap& eventData)
{
    Connection* serverConnection = DV_NET.GetServerConnection();
    // If we were connected to server, disconnect. Or if we were running a server, stop it. In both cases clear the
    // scene of all replicated content, but let the local nodes & components (the static world + camera) stay
    if (serverConnection)
    {
        serverConnection->Disconnect();
        scene_->Clear(true, false);
        clientObjectID_ = 0;
    }
    // Or if we were running a server, stop it
    else if (DV_NET.IsServerRunning())
    {
        DV_NET.StopServer();
        scene_->Clear(true, false);
    }

    UpdateButtons();
}

void SceneReplication::HandleStartServer(StringHash eventType, VariantMap& eventData)
{
    DV_NET.StartServer(SERVER_PORT);

    UpdateButtons();
}

void SceneReplication::HandleConnectionStatus(StringHash eventType, VariantMap& eventData)
{
    UpdateButtons();
}

void SceneReplication::HandleClientConnected(StringHash eventType, VariantMap& eventData)
{
    using namespace ClientConnected;

    // When a client connects, assign to scene to begin scene replication
    auto* newConnection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
    newConnection->SetScene(scene_);

    // Then create a controllable object for that client
    Node* newObject = CreateControllableObject();
    serverObjects_[newConnection] = newObject;

    // Finally send the object's node ID using a remote event
    VariantMap remoteEventData;
    remoteEventData[P_ID] = newObject->GetID();
    newConnection->SendRemoteEvent(E_CLIENTOBJECTID, true, remoteEventData);
}

void SceneReplication::HandleClientDisconnected(StringHash eventType, VariantMap& eventData)
{
    using namespace ClientConnected;

    // When a client disconnects, remove the controlled object
    auto* connection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
    Node* object = serverObjects_[connection];
    if (object)
        object->Remove();

    serverObjects_.Erase(connection);
}

void SceneReplication::HandleClientObjectID(StringHash eventType, VariantMap& eventData)
{
    clientObjectID_ = eventData[P_ID].GetU32();
}
