// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/containers/vector.h>
#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/debug_renderer.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/input/input.h>
#include <dviglo/io/file_system.h>
#include <dviglo/physics_2d/collision_box_2d.h>
#include <dviglo/physics_2d/collision_circle_2d.h>
#include <dviglo/physics_2d/collision_edge_2d.h>
#include <dviglo/physics_2d/collision_polygon_2d.h>
#include <dviglo/physics_2d/constraint_distance_2d.h>
#include <dviglo/physics_2d/constraint_friction_2d.h>
#include <dviglo/physics_2d/constraint_gear_2d.h>
#include <dviglo/physics_2d/constraint_motor_2d.h>
#include <dviglo/physics_2d/constraint_mouse_2d.h>
#include <dviglo/physics_2d/constraint_prismatic_2d.h>
#include <dviglo/physics_2d/constraint_pulley_2d.h>
#include <dviglo/physics_2d/constraint_revolute_2d.h>
#include <dviglo/physics_2d/constraint_weld_2d.h>
#include <dviglo/physics_2d/constraint_wheel_2d.h>
#include <dviglo/physics_2d/physics_world_2d.h>
#include <dviglo/physics_2d/rigid_body_2d.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/scene/scene_events.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/text3d.h>
#include <dviglo/urho_2d/drawable_2d.h>
#include <dviglo/urho_2d/sprite_2d.h>
#include <dviglo/urho_2d/static_sprite_2d.h>

#include "constraints.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(Urho2DConstraints)

Node* pickedNode;
RigidBody2D* dummyBody;

Urho2DConstraints::Urho2DConstraints()
{
}

void Urho2DConstraints::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    create_scene();

    // Enable OS cursor
    DV_INPUT.SetMouseVisible(true);

    // Create the UI content
    create_instructions();

    // Hook up to the frame update events
    subscribe_to_events();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void Urho2DConstraints::create_scene()
{
    scene_ = new Scene();
    scene_->create_component<Octree>();
    scene_->create_component<DebugRenderer>();
    auto* physicsWorld = scene_->create_component<PhysicsWorld2D>(); // Create 2D physics world component
    physicsWorld->SetDrawJoint(true); // Display the joints (Note that draw_debug_geometry() must be set to true to acually draw the joints)
    drawDebug_ = true; // Set draw_debug_geometry() to true

    // Create camera
    cameraNode_ = scene_->create_child("Camera");
    // Set camera's position
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, 0.0f)); // Note that Z setting is discarded; use camera.zoom instead (see move_camera() below for example)

    camera_ = cameraNode_->create_component<Camera>();
    camera_->SetOrthographic(true);

    camera_->SetOrthoSize((float)DV_GRAPHICS.GetHeight() * PIXEL_SIZE);
    camera_->SetZoom(1.2f * Min((float)DV_GRAPHICS.GetWidth() / 1280.0f, (float)DV_GRAPHICS.GetHeight() / 800.0f)); // Set zoom according to user's resolution to ensure full visibility (initial zoom (1.2) is set for full visibility at 1280x800 resolution)

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, camera_));
    DV_RENDERER.SetViewport(0, viewport);

    Zone* zone = DV_RENDERER.GetDefaultZone();
    zone->SetFogColor(Color(0.1f, 0.1f, 0.1f)); // Set background color for the scene

    // Create 4x3 grid
    for (unsigned i = 0; i<5; ++i)
    {
        Node* edgeNode = scene_->create_child("VerticalEdge");
        auto* edgeBody = edgeNode->create_component<RigidBody2D>();
        if (!dummyBody)
            dummyBody = edgeBody; // Mark first edge as dummy body (used by mouse pick)
        auto* edgeShape = edgeNode->create_component<CollisionEdge2D>();
        edgeShape->SetVertices(Vector2(i*2.5f -5.0f, -3.0f), Vector2(i*2.5f -5.0f, 3.0f));
        edgeShape->SetFriction(0.5f); // Set friction
    }

    for (unsigned j = 0; j<4; ++j)
    {
        Node* edgeNode = scene_->create_child("HorizontalEdge");
        /*RigidBody2D* edgeBody = */edgeNode->create_component<RigidBody2D>();
        auto* edgeShape = edgeNode->create_component<CollisionEdge2D>();
        edgeShape->SetVertices(Vector2(-5.0f, j*2.0f -3.0f), Vector2(5.0f, j*2.0f -3.0f));
        edgeShape->SetFriction(0.5f); // Set friction
    }

    // Create a box (will be cloned later)
    Node* box  = scene_->create_child("Box");
    box->SetPosition(Vector3(0.8f, -2.0f, 0.0f));
    auto* boxSprite = box->create_component<StaticSprite2D>();
    boxSprite->SetSprite(DV_RES_CACHE.GetResource<Sprite2D>("Urho2D/Box.png"));
    auto* boxBody = box->create_component<RigidBody2D>();
    boxBody->SetBodyType(BT_DYNAMIC);
    boxBody->SetLinearDamping(0.0f);
    boxBody->SetAngularDamping(0.0f);
    auto* shape = box->create_component<CollisionBox2D>(); // Create box shape
    shape->SetSize(Vector2(0.32f, 0.32f)); // Set size
    shape->SetDensity(1.0f); // Set shape density (kilograms per meter squared)
    shape->SetFriction(0.5f); // Set friction
    shape->SetRestitution(0.1f); // Set restitution (slight bounce)

    // Create a ball (will be cloned later)
    Node* ball  = scene_->create_child("Ball");
    ball->SetPosition(Vector3(1.8f, -2.0f, 0.0f));
    auto* ballSprite = ball->create_component<StaticSprite2D>();
    ballSprite->SetSprite(DV_RES_CACHE.GetResource<Sprite2D>("Urho2D/Ball.png"));
    auto* ballBody = ball->create_component<RigidBody2D>();
    ballBody->SetBodyType(BT_DYNAMIC);
    ballBody->SetLinearDamping(0.0f);
    ballBody->SetAngularDamping(0.0f);
    auto* ballShape = ball->create_component<CollisionCircle2D>(); // Create circle shape
    ballShape->SetRadius(0.16f); // Set radius
    ballShape->SetDensity(1.0f); // Set shape density (kilograms per meter squared)
    ballShape->SetFriction(0.5f); // Set friction
    ballShape->SetRestitution(0.6f); // Set restitution: make it bounce

    // Create a polygon
    Node* polygon = scene_->create_child("Polygon");
    polygon->SetPosition(Vector3(1.6f, -2.0f, 0.0f));
    polygon->SetScale(0.7f);
    auto* polygonSprite = polygon->create_component<StaticSprite2D>();
    polygonSprite->SetSprite(DV_RES_CACHE.GetResource<Sprite2D>("Urho2D/Aster.png"));
    auto* polygonBody = polygon->create_component<RigidBody2D>();
    polygonBody->SetBodyType(BT_DYNAMIC);
    auto* polygonShape = polygon->create_component<CollisionPolygon2D>();
    // TODO: create from Vector<Vector2> using SetVertices()
    polygonShape->SetVertexCount(6); // Set number of vertices (mandatory when using SetVertex())
    polygonShape->SetVertex(0, Vector2(-0.8f, -0.3f));
    polygonShape->SetVertex(1, Vector2(0.5f, -0.8f));
    polygonShape->SetVertex(2, Vector2(0.8f, -0.3f));
    polygonShape->SetVertex(3, Vector2(0.8f, 0.5f));
    polygonShape->SetVertex(4, Vector2(0.5f, 0.9f));
    polygonShape->SetVertex(5, Vector2(-0.5f, 0.7f));
    polygonShape->SetDensity(1.0f); // Set shape density (kilograms per meter squared)
    polygonShape->SetFriction(0.3f); // Set friction
    polygonShape->SetRestitution(0.0f); // Set restitution (no bounce)

    // Create a ConstraintDistance2D
    CreateFlag("ConstraintDistance2D", -4.97f, 3.0f); // Display Text3D flag
    Node* boxDistanceNode = box->Clone();
    Node* ballDistanceNode = ball->Clone();
    auto* ballDistanceBody = ballDistanceNode->GetComponent<RigidBody2D>();
    boxDistanceNode->SetPosition(Vector3(-4.5f, 2.0f, 0.0f));
    ballDistanceNode->SetPosition(Vector3(-3.0f, 2.0f, 0.0f));

    auto* constraintDistance = boxDistanceNode->create_component<ConstraintDistance2D>(); // Apply ConstraintDistance2D to box
    constraintDistance->SetOtherBody(ballDistanceBody); // Constrain ball to box
    constraintDistance->SetOwnerBodyAnchor(boxDistanceNode->GetPosition2D());
    constraintDistance->SetOtherBodyAnchor(ballDistanceNode->GetPosition2D());
    
    // Make the constraint soft (comment to make it rigid, which is its basic behavior)
    constraintDistance->SetMinLength(constraintDistance->GetLength() - 1.f);
    constraintDistance->SetMaxLength(constraintDistance->GetLength() + 1.f);
    constraintDistance->SetLinearStiffness(4.0f, 0.5f);

    // Create a ConstraintFriction2D ********** Not functional. From Box2d samples it seems that 2 anchors are required, Urho2D only provides 1, needs investigation ***********
    CreateFlag("ConstraintFriction2D", 0.03f, 1.0f); // Display Text3D flag
    Node* boxFrictionNode = box->Clone();
    Node* ballFrictionNode = ball->Clone();
    boxFrictionNode->SetPosition(Vector3(0.5f, 0.0f, 0.0f));
    ballFrictionNode->SetPosition(Vector3(1.5f, 0.0f, 0.0f));

    auto* constraintFriction = boxFrictionNode->create_component<ConstraintFriction2D>(); // Apply ConstraintDistance2D to box
    constraintFriction->SetOtherBody(ballFrictionNode->GetComponent<RigidBody2D>()); // Constraint ball to box
    //constraintFriction->SetOwnerBodyAnchor(boxNode->GetPosition2D());
    //constraintFriction->SetOtherBodyAnchor(ballNode->GetPosition2D());
    //constraintFriction->SetMaxForce(10.0f); // ballBody.mass * gravity
    //constraintDistance->SetMaxTorque(10.0f); // ballBody.mass * radius * gravity

    // Create a ConstraintGear2D
    CreateFlag("ConstraintGear2D", -4.97f, -1.0f); // Display Text3D flag
    Node* baseNode = box->Clone();
    auto* tempBody = baseNode->GetComponent<RigidBody2D>(); // Get body to make it static
    tempBody->SetBodyType(BT_STATIC);
    baseNode->SetPosition(Vector3(-3.7f, -2.5f, 0.0f));
    Node* ball1Node = ball->Clone();
    ball1Node->SetPosition(Vector3(-4.5f, -2.0f, 0.0f));
    auto* ball1Body = ball1Node->GetComponent<RigidBody2D>();
    Node* ball2Node = ball->Clone();
    ball2Node->SetPosition(Vector3(-3.0f, -2.0f, 0.0f));
    auto* ball2Body = ball2Node->GetComponent<RigidBody2D>();

    auto* gear1 = baseNode->create_component<ConstraintRevolute2D>(); // Apply constraint to baseBox
    gear1->SetOtherBody(ball1Body); // Constrain ball1 to baseBox
    gear1->SetAnchor(ball1Node->GetPosition2D());
    auto* gear2 = baseNode->create_component<ConstraintRevolute2D>(); // Apply constraint to baseBox
    gear2->SetOtherBody(ball2Body); // Constrain ball2 to baseBox
    gear2->SetAnchor(ball2Node->GetPosition2D());

    auto* constraintGear = ball1Node->create_component<ConstraintGear2D>(); // Apply constraint to ball1
    constraintGear->SetOtherBody(ball2Body); // Constrain ball2 to ball1
    constraintGear->SetOwnerConstraint(gear1);
    constraintGear->SetOtherConstraint(gear2);
    constraintGear->SetRatio(1.0f);

    ball1Body->ApplyAngularImpulse(0.015f, true); // Animate

    // Create a vehicle from a compound of 2 ConstraintWheel2Ds
    CreateFlag("ConstraintWheel2Ds compound", -2.45f, -1.0f); // Display Text3D flag
    Node* car = box->Clone();
    car->SetScale(Vector3(4.0f, 1.0f, 0.0f));
    car->SetPosition(Vector3(-1.2f, -2.3f, 0.0f));
    auto* tempSprite = car->GetComponent<StaticSprite2D>(); // Get car Sprite in order to draw it on top
    tempSprite->SetOrderInLayer(0); // Draw car on top of the wheels (set to -1 to draw below)
    Node* ball1WheelNode = ball->Clone();
    ball1WheelNode->SetPosition(Vector3(-1.6f, -2.5f, 0.0f));
    Node* ball2WheelNode = ball->Clone();
    ball2WheelNode->SetPosition(Vector3(-0.8f, -2.5f, 0.0f));

    auto* wheel1 = car->create_component<ConstraintWheel2D>();
    wheel1->SetOtherBody(ball1WheelNode->GetComponent<RigidBody2D>());
    wheel1->SetAnchor(ball1WheelNode->GetPosition2D());
    wheel1->SetAxis(Vector2(0.0f, 1.0f));
    wheel1->SetMaxMotorTorque(20.0f);
    wheel1->SetLinearStiffness(4.0f, 0.4f);

    auto* wheel2 = car->create_component<ConstraintWheel2D>();
    wheel2->SetOtherBody(ball2WheelNode->GetComponent<RigidBody2D>());
    wheel2->SetAnchor(ball2WheelNode->GetPosition2D());
    wheel2->SetAxis(Vector2(0.0f, 1.0f));
    wheel2->SetMaxMotorTorque(10.0f);
    wheel2->SetLinearStiffness(4.0f, 0.4f);

    // ConstraintMotor2D
    CreateFlag("ConstraintMotor2D", 2.53f, -1.0f); // Display Text3D flag
    Node* boxMotorNode = box->Clone();
    tempBody = boxMotorNode->GetComponent<RigidBody2D>(); // Get body to make it static
    tempBody->SetBodyType(BT_STATIC);
    Node* ballMotorNode = ball->Clone();
    boxMotorNode->SetPosition(Vector3(3.8f, -2.1f, 0.0f));
    ballMotorNode->SetPosition(Vector3(3.8f, -1.5f, 0.0f));

    auto* constraintMotor = boxMotorNode->create_component<ConstraintMotor2D>();
    constraintMotor->SetOtherBody(ballMotorNode->GetComponent<RigidBody2D>()); // Constrain ball to box
    constraintMotor->SetLinearOffset(Vector2(0.0f, 0.8f)); // Set ballNode position relative to boxNode position = (0,0)
    constraintMotor->SetAngularOffset(0.1f);
    constraintMotor->SetMaxForce(5.0f);
    constraintMotor->SetMaxTorque(10.0f);
    constraintMotor->SetCorrectionFactor(1.0f);
    constraintMotor->SetCollideConnected(true); // doesn't work

    // ConstraintMouse2D is demonstrated in HandleMouseButtonDown() function. It is used to "grasp" the sprites with the mouse.
    CreateFlag("ConstraintMouse2D", 0.03f, -1.0f); // Display Text3D flag

    // Create a ConstraintPrismatic2D
    CreateFlag("ConstraintPrismatic2D", 2.53f, 3.0f); // Display Text3D flag
    Node* boxPrismaticNode = box->Clone();
    tempBody = boxPrismaticNode->GetComponent<RigidBody2D>(); // Get body to make it static
    tempBody->SetBodyType(BT_STATIC);
    Node* ballPrismaticNode = ball->Clone();
    boxPrismaticNode->SetPosition(Vector3(3.3f, 2.5f, 0.0f));
    ballPrismaticNode->SetPosition(Vector3(4.3f, 2.0f, 0.0f));

    auto* constraintPrismatic = boxPrismaticNode->create_component<ConstraintPrismatic2D>();
    constraintPrismatic->SetOtherBody(ballPrismaticNode->GetComponent<RigidBody2D>()); // Constrain ball to box
    constraintPrismatic->SetAxis(Vector2(1.0f, 1.0f)); // Slide from [0,0] to [1,1]
    constraintPrismatic->SetAnchor(Vector2(4.0f, 2.0f));
    constraintPrismatic->SetLowerTranslation(-1.0f);
    constraintPrismatic->SetUpperTranslation(0.5f);
    constraintPrismatic->SetEnableLimit(true);
    constraintPrismatic->SetMaxMotorForce(1.0f);
    constraintPrismatic->SetMotorSpeed(0.0f);

    // ConstraintPulley2D
    CreateFlag("ConstraintPulley2D", 0.03f, 3.0f); // Display Text3D flag
    Node* boxPulleyNode = box->Clone();
    Node* ballPulleyNode = ball->Clone();
    boxPulleyNode->SetPosition(Vector3(0.5f, 2.0f, 0.0f));
    ballPulleyNode->SetPosition(Vector3(2.0f, 2.0f, 0.0f));

    auto* constraintPulley = boxPulleyNode->create_component<ConstraintPulley2D>(); // Apply constraint to box
    constraintPulley->SetOtherBody(ballPulleyNode->GetComponent<RigidBody2D>()); // Constrain ball to box
    constraintPulley->SetOwnerBodyAnchor(boxPulleyNode->GetPosition2D());
    constraintPulley->SetOtherBodyAnchor(ballPulleyNode->GetPosition2D());
    constraintPulley->SetOwnerBodyGroundAnchor(boxPulleyNode->GetPosition2D() + Vector2(0.0f, 1.0f));
    constraintPulley->SetOtherBodyGroundAnchor(ballPulleyNode->GetPosition2D() + Vector2(0.0f, 1.0f));
    constraintPulley->SetRatio(1.0); // Weight ratio between ownerBody and otherBody

    // Create a ConstraintRevolute2D
    CreateFlag("ConstraintRevolute2D", -2.45f, 3.0f); // Display Text3D flag
    Node* boxRevoluteNode = box->Clone();
    tempBody = boxRevoluteNode->GetComponent<RigidBody2D>(); // Get body to make it static
    tempBody->SetBodyType(BT_STATIC);
    Node* ballRevoluteNode = ball->Clone();
    boxRevoluteNode->SetPosition(Vector3(-2.0f, 1.5f, 0.0f));
    ballRevoluteNode->SetPosition(Vector3(-1.0f, 2.0f, 0.0f));

    auto* constraintRevolute = boxRevoluteNode->create_component<ConstraintRevolute2D>(); // Apply constraint to box
    constraintRevolute->SetOtherBody(ballRevoluteNode->GetComponent<RigidBody2D>()); // Constrain ball to box
    constraintRevolute->SetAnchor(Vector2(-1.0f, 1.5f));
    constraintRevolute->SetLowerAngle(-1.0f); // In radians
    constraintRevolute->SetUpperAngle(0.5f); // In radians
    constraintRevolute->SetEnableLimit(true);
    constraintRevolute->SetMaxMotorTorque(10.0f);
    constraintRevolute->SetMotorSpeed(0.0f);
    constraintRevolute->SetEnableMotor(true);

    // Create a ConstraintWeld2D
    CreateFlag("ConstraintWeld2D", -2.45f, 1.0f); // Display Text3D flag
    Node* boxWeldNode = box->Clone();
    Node* ballWeldNode = ball->Clone();
    boxWeldNode->SetPosition(Vector3(-0.5f, 0.0f, 0.0f));
    ballWeldNode->SetPosition(Vector3(-2.0f, 0.0f, 0.0f));

    auto* constraintWeld = boxWeldNode->create_component<ConstraintWeld2D>();
    constraintWeld->SetOtherBody(ballWeldNode->GetComponent<RigidBody2D>()); // Constrain ball to box
    constraintWeld->SetAnchor(boxWeldNode->GetPosition2D());
    constraintWeld->SetAngularStiffness(4.0f, 0.5f);

    // Create a ConstraintWheel2D
    CreateFlag("ConstraintWheel2D",  2.53f, 1.0f); // Display Text3D flag
    Node* boxWheelNode = box->Clone();
    Node* ballWheelNode = ball->Clone();
    boxWheelNode->SetPosition(Vector3(3.8f, 0.0f, 0.0f));
    ballWheelNode->SetPosition(Vector3(3.8f, 0.9f, 0.0f));

    auto* constraintWheel = boxWheelNode->create_component<ConstraintWheel2D>();
    constraintWheel->SetOtherBody(ballWheelNode->GetComponent<RigidBody2D>()); // Constrain ball to box
    constraintWheel->SetAnchor(ballWheelNode->GetPosition2D());
    constraintWheel->SetAxis(Vector2(0.0f, 1.0f));
    constraintWheel->SetEnableMotor(true);
    constraintWheel->SetMaxMotorTorque(1.0f);
    constraintWheel->SetMotorSpeed(0.0f);
    constraintWheel->SetLinearStiffness(4.0f, 0.5f);
    //constraintWheel->SetCollideConnected(true); // doesn't work
}

void Urho2DConstraints::CreateFlag(const String& text, float x, float y) // Used to create Tex3D flags
{
    Node* flagNode = scene_->create_child("Flag");
    flagNode->SetPosition(Vector3(x, y, 0.0f));
    auto* flag3D = flagNode->create_component<Text3D>(); // We use Text3D in order to make the text affected by zoom (so that it sticks to 2D)
    flag3D->SetText(text);
    flag3D->SetFont(DV_RES_CACHE.GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
}

void Urho2DConstraints::create_instructions()
{
    // Construct new Text object, set string to display and font to use
    auto* instructionText = DV_UI.GetRoot()->create_child<Text>();
    instructionText->SetText("Use WASD keys and mouse to move, Use PageUp PageDown to zoom.\n Space to toggle debug geometry and joints - F5 to save the scene.");
    instructionText->SetFont(DV_RES_CACHE.GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    instructionText->SetTextAlignment(HA_CENTER); // Center rows in relation to each other

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI.GetRoot()->GetHeight() / 4);
}

void Urho2DConstraints::move_camera(float timeStep)
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
        camera_->SetZoom(camera_->GetZoom() * 1.01f);

    if (input.GetKeyDown(KEY_PAGEDOWN))
        camera_->SetZoom(camera_->GetZoom() * 0.99f);
}

void Urho2DConstraints::subscribe_to_events()
{
    // Subscribe handle_update() function for processing update events
    subscribe_to_event(E_UPDATE, DV_HANDLER(Urho2DConstraints, handle_update));

    // Subscribe handle_post_render_update() function for processing the post-render update event, during which we request debug geometry
    subscribe_to_event(E_POSTRENDERUPDATE, DV_HANDLER(Urho2DConstraints, handle_post_render_update));

    // Subscribe to mouse click
    subscribe_to_event(E_MOUSEBUTTONDOWN, DV_HANDLER(Urho2DConstraints, HandleMouseButtonDown));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    unsubscribe_from_event(E_SCENEUPDATE);
}

void Urho2DConstraints::handle_update(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    move_camera(timeStep);

    // Toggle physics debug geometry with space
    if (DV_INPUT.GetKeyPress(KEY_SPACE))
        drawDebug_ = !drawDebug_;

    // Save scene
    if (DV_INPUT.GetKeyPress(KEY_F5))
    {
        File saveFile(DV_FILE_SYSTEM.GetProgramDir() + "Data/Scenes/Constraints.xml", FILE_WRITE);
        scene_->save_xml(saveFile);
    }
}

void Urho2DConstraints::handle_post_render_update(StringHash eventType, VariantMap& eventData)
{
    auto* physicsWorld = scene_->GetComponent<PhysicsWorld2D>();
    if (drawDebug_) physicsWorld->draw_debug_geometry();
}

void Urho2DConstraints::HandleMouseButtonDown(StringHash eventType, VariantMap& eventData)
{
    auto* physicsWorld = scene_->GetComponent<PhysicsWorld2D>();
    RigidBody2D* rigidBody = physicsWorld->GetRigidBody(DV_INPUT.GetMousePosition().x, DV_INPUT.GetMousePosition().y); // Raycast for RigidBody2Ds to pick
    if (rigidBody)
    {
        pickedNode = rigidBody->GetNode();
        //log.Info(pickedNode.name);
        auto* staticSprite = pickedNode->GetComponent<StaticSprite2D>();
        staticSprite->SetColor(Color(1.0f, 0.0f, 0.0f, 1.0f)); // Temporary modify color of the picked sprite

        // Create a ConstraintMouse2D - Temporary apply this constraint to the pickedNode to allow grasping and moving with the mouse
        auto* constraintMouse = pickedNode->create_component<ConstraintMouse2D>();
        constraintMouse->SetTarget(GetMousePositionXY());
        constraintMouse->SetMaxForce(1000 * rigidBody->GetMass());
        constraintMouse->SetCollideConnected(true);
        constraintMouse->SetOtherBody(dummyBody);  // Use dummy body instead of rigidBody. It's better to create a dummy body automatically in ConstraintMouse2D
        constraintMouse->SetLinearStiffness(5.0f, 0.7f);
    }
    subscribe_to_event(E_MOUSEMOVE, DV_HANDLER(Urho2DConstraints, HandleMouseMove));
    subscribe_to_event(E_MOUSEBUTTONUP, DV_HANDLER(Urho2DConstraints, HandleMouseButtonUp));
}

void Urho2DConstraints::HandleMouseButtonUp(StringHash eventType, VariantMap& eventData)
{
    if (pickedNode)
    {
        auto* staticSprite = pickedNode->GetComponent<StaticSprite2D>();
        staticSprite->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f)); // Restore picked sprite color

        pickedNode->RemoveComponent<ConstraintMouse2D>(); // Remove temporary constraint
        pickedNode = nullptr;
    }
    unsubscribe_from_event(E_MOUSEMOVE);
    unsubscribe_from_event(E_MOUSEBUTTONUP);
}

Vector2 Urho2DConstraints::GetMousePositionXY()
{
    Vector3 screenPoint = Vector3((float)DV_INPUT.GetMousePosition().x / DV_GRAPHICS.GetWidth(), (float)DV_INPUT.GetMousePosition().y / DV_GRAPHICS.GetHeight(), 0.0f);
    Vector3 worldPoint = camera_->screen_to_world_point(screenPoint);
    return Vector2(worldPoint.x, worldPoint.y);
}

void Urho2DConstraints::HandleMouseMove(StringHash eventType, VariantMap& eventData)
{
    if (pickedNode)
    {
        auto* constraintMouse = pickedNode->GetComponent<ConstraintMouse2D>();
        constraintMouse->SetTarget(GetMousePositionXY());
    }
}
