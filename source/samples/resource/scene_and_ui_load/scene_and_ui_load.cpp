// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/input/input.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/button.h>
#include <dviglo/ui/ui.h>
#include <dviglo/ui/ui_events.h>

#include "scene_and_ui_load.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(SceneAndUILoad)

SceneAndUILoad::SceneAndUILoad()
{
}

void SceneAndUILoad::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateUI();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Subscribe to global events for camera movement
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void SceneAndUILoad::CreateScene()
{
    scene_ = new Scene();

    // Load scene content prepared in the editor (XML format). GetFile() returns an open file from the resource system
    // which scene.load_xml() will read
    SharedPtr<File> file = DV_RES_CACHE.GetFile("Scenes/SceneLoadExample.xml");
    scene_->load_xml(*file);

    // Create the camera (not included in the scene file)
    cameraNode_ = scene_->create_child("Camera");
    cameraNode_->create_component<Camera>();

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 2.0f, -10.0f));
}

void SceneAndUILoad::CreateUI()
{
    // Set up global UI style into the root UI element
    auto* style = DV_RES_CACHE.GetResource<XmlFile>("UI/DefaultStyle.xml");
    DV_UI.GetRoot()->SetDefaultStyle(style);

    // Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will
    // control the camera, and when visible, it will interact with the UI
    SharedPtr<Cursor> cursor(new Cursor());
    cursor->SetStyleAuto();
    DV_UI.SetCursor(cursor);
    // Set starting position of the cursor at the rendering window center
    cursor->SetPosition(DV_GRAPHICS.GetWidth() / 2, DV_GRAPHICS.GetHeight() / 2);

    // Load UI content prepared in the editor and add to the UI hierarchy
    SharedPtr<UiElement> layoutRoot = DV_UI.LoadLayout(DV_RES_CACHE.GetResource<XmlFile>("UI/UILoadExample.xml"));
    DV_UI.GetRoot()->AddChild(layoutRoot);

    // Subscribe to button actions (toggle scene lights when pressed then released)
    auto* button = layoutRoot->GetChildStaticCast<Button>("ToggleLight1", true);
    if (button)
        subscribe_to_event(button, E_RELEASED, DV_HANDLER(SceneAndUILoad, ToggleLight1));
    button = layoutRoot->GetChildStaticCast<Button>("ToggleLight2", true);
    if (button)
        subscribe_to_event(button, E_RELEASED, DV_HANDLER(SceneAndUILoad, ToggleLight2));
}

void SceneAndUILoad::SetupViewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    DV_RENDERER.SetViewport(0, viewport);
}

void SceneAndUILoad::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for camera motion
    subscribe_to_event(E_UPDATE, DV_HANDLER(SceneAndUILoad, HandleUpdate));
}

void SceneAndUILoad::MoveCamera(float timeStep)
{
    // Right mouse button controls mouse cursor visibility: hide when pressed
    Input& input = DV_INPUT;
    DV_UI.GetCursor()->SetVisible(!input.GetMouseButtonDown(MOUSEB_RIGHT));

    // Do not move if the UI has a focused element
    if (DV_UI.GetFocusElement())
        return;

    // Movement speed as world units per second
    const float MOVE_SPEED = 20.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    // Only move the camera when the cursor is hidden
    if (!DV_UI.GetCursor()->IsVisible())
    {
        IntVector2 mouseMove = input.GetMouseMove();
        yaw_ += MOUSE_SENSITIVITY * mouseMove.x;
        pitch_ += MOUSE_SENSITIVITY * mouseMove.y;
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);

        // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
        cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
    }

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input.GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
}

void SceneAndUILoad::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}

void SceneAndUILoad::ToggleLight1(StringHash eventType, VariantMap& eventData)
{
    Node* lightNode = scene_->GetChild("Light1", true);
    if (lightNode)
        lightNode->SetEnabled(!lightNode->IsEnabled());
}

void SceneAndUILoad::ToggleLight2(StringHash eventType, VariantMap& eventData)
{
    Node* lightNode = scene_->GetChild("Light2", true);
    if (lightNode)
        lightNode->SetEnabled(!lightNode->IsEnabled());
}
