// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/debug_renderer.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/input/input.h>
#include <dviglo/physics_2d/collision_box_2d.h>
#include <dviglo/physics_2d/collision_circle_2d.h>
#include <dviglo/physics_2d/physics_world_2d.h>
#include <dviglo/physics_2d/rigid_body_2d.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/scene/scene_events.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/urho_2d/drawable_2d.h>
#include <dviglo/urho_2d/sprite_2d.h>
#include <dviglo/urho_2d/static_sprite_2d.h>

#include "hello.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(Urho2DPhysics)

static const unsigned NUM_OBJECTS = 100;

Urho2DPhysics::Urho2DPhysics()
{
}

void Urho2DPhysics::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    create_scene();

    // Create the UI content
    create_instructions();

    // Setup the viewport for displaying the scene
    setup_viewport();

    // Hook up to the frame update events
    subscribe_to_events();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void Urho2DPhysics::create_scene()
{
    scene_ = new Scene();
    scene_->create_component<Octree>();
    scene_->create_component<DebugRenderer>();
    // Create camera node
    cameraNode_ = scene_->create_child("Camera");
    // Set camera's position
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -10.0f));

    auto* camera = cameraNode_->create_component<Camera>();
    camera->SetOrthographic(true);

    camera->SetOrthoSize((float)DV_GRAPHICS.GetHeight() * PIXEL_SIZE);
    camera->SetZoom(1.2f * Min((float)DV_GRAPHICS.GetWidth() / 1280.0f, (float)DV_GRAPHICS.GetHeight() / 800.0f)); // Set zoom according to user's resolution to ensure full visibility (initial zoom (1.2) is set for full visibility at 1280x800 resolution)

    // Create 2D physics world component
    /*PhysicsWorld2D* physicsWorld = */scene_->create_component<PhysicsWorld2D>();

    auto* boxSprite = DV_RES_CACHE.GetResource<Sprite2D>("sprites/Box.png");
    auto* ballSprite = DV_RES_CACHE.GetResource<Sprite2D>("sprites/Ball.png");

    // Create ground.
    Node* groundNode = scene_->create_child("Ground");
    groundNode->SetPosition(Vector3(0.0f, -3.0f, 0.0f));
    groundNode->SetScale(Vector3(200.0f, 1.0f, 0.0f));

    // Create 2D rigid body for gound
    /*RigidBody2D* groundBody = */groundNode->create_component<RigidBody2D>();

    auto* groundSprite = groundNode->create_component<StaticSprite2D>();
    groundSprite->SetSprite(boxSprite);

    // Create box collider for ground
    auto* groundShape = groundNode->create_component<CollisionBox2D>();
    // Set box size
    groundShape->SetSize(Vector2(0.32f, 0.32f));
    // Set friction
    groundShape->SetFriction(0.5f);

    for (unsigned i = 0; i < NUM_OBJECTS; ++i)
    {
        Node* node  = scene_->create_child("RigidBody");
        node->SetPosition(Vector3(Random(-0.1f, 0.1f), 5.0f + i * 0.4f, 0.0f));

        // Create rigid body
        auto* body = node->create_component<RigidBody2D>();
        body->SetBodyType(BT_DYNAMIC);

        auto* staticSprite = node->create_component<StaticSprite2D>();

        if (i % 2 == 0)
        {
            staticSprite->SetSprite(boxSprite);

            // Create box
            auto* box = node->create_component<CollisionBox2D>();
            // Set size
            box->SetSize(Vector2(0.32f, 0.32f));
            // Set density
            box->SetDensity(1.0f);
            // Set friction
            box->SetFriction(0.5f);
            // Set restitution
            box->SetRestitution(0.1f);
        }
        else
        {
            staticSprite->SetSprite(ballSprite);

            // Create circle
            auto* circle = node->create_component<CollisionCircle2D>();
            // Set radius
            circle->SetRadius(0.16f);
            // Set density
            circle->SetDensity(1.0f);
            // Set friction.
            circle->SetFriction(0.5f);
            // Set restitution
            circle->SetRestitution(0.1f);
        }
    }
}

void Urho2DPhysics::create_instructions()
{
    // Construct new Text object, set string to display and font to use
    auto* instructionText = DV_UI.GetRoot()->create_child<Text>();
    instructionText->SetText("Use WASD keys to move, use PageUp PageDown keys to zoom.");
    instructionText->SetFont(DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI.GetRoot()->GetHeight() / 4);
}

void Urho2DPhysics::setup_viewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    DV_RENDERER.SetViewport(0, viewport);
}

void Urho2DPhysics::move_camera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (DV_UI.GetFocusElement())
        return;

    Input& input = DV_INPUT;

    // Movement speed as world units per second
    const float MOVE_SPEED = 4.0f;

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input.GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::UP * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::DOWN * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

    if (input.GetKeyDown(KEY_PAGEUP))
    {
        auto* camera = cameraNode_->GetComponent<Camera>();
        camera->SetZoom(camera->GetZoom() * 1.01f);
    }

    if (input.GetKeyDown(KEY_PAGEDOWN))
    {
        auto* camera = cameraNode_->GetComponent<Camera>();
        camera->SetZoom(camera->GetZoom() * 0.99f);
    }
}

void Urho2DPhysics::subscribe_to_events()
{
    // Subscribe handle_update() function for processing update events
    subscribe_to_event(E_UPDATE, DV_HANDLER(Urho2DPhysics, handle_update));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    unsubscribe_from_event(E_SCENEUPDATE);
}

void Urho2DPhysics::handle_update(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    move_camera(timeStep);
}
