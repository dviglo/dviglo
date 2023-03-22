// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/animated_model.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/debug_renderer.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/light.h>
#include <dviglo/graphics/material.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/input/input.h>
#include <dviglo/io/file.h>
#include <dviglo/io/file_system.h>
#include <dviglo/physics/collision_shape.h>
#include <dviglo/physics/physics_world.h>
#include <dviglo/physics/rigid_body.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

#include "create_ragdoll.h"
#include "ragdolls.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(Ragdolls)

Ragdolls::Ragdolls() :
    drawDebug_(false)
{
    // Register an object factory for our custom CreateRagdoll component so that we can create them to scene nodes
    DV_CONTEXT.RegisterFactory<CreateRagdoll>();
}

void Ragdolls::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update and render post-update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_ABSOLUTE);
}

void Ragdolls::CreateScene()
{
    ResourceCache& cache = DV_RES_CACHE;

    scene_ = new Scene();

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    // Create a physics simulation world with default parameters, which will update at 60fps. Like the Octree must
    // exist before creating drawable components, the PhysicsWorld must exist before creating physics components.
    // Finally, create a DebugRenderer component so that we can draw physics debug geometry
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<PhysicsWorld>();
    scene_->CreateComponent<DebugRenderer>();

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode = scene_->CreateChild("Zone");
    auto* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
    zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create a directional light to the world. Enable cascaded shadows on it
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    auto* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));

    {
        // Create a floor object, 500 x 500 world units. Adjust position so that the ground is at zero Y
        Node* floorNode = scene_->CreateChild("Floor");
        floorNode->SetPosition(Vector3(0.0f, -0.5f, 0.0f));
        floorNode->SetScale(Vector3(500.0f, 1.0f, 500.0f));
        auto* floorObject = floorNode->CreateComponent<StaticModel>();
        floorObject->SetModel(cache.GetResource<Model>("Models/Box.mdl"));
        floorObject->SetMaterial(cache.GetResource<Material>("Materials/StoneTiled.xml"));

        // Make the floor physical by adding RigidBody and CollisionShape components
        auto* body = floorNode->CreateComponent<RigidBody>();
        // We will be spawning spherical objects in this sample. The ground also needs non-zero rolling friction so that
        // the spheres will eventually come to rest
        body->SetRollingFriction(0.15f);
        auto* shape = floorNode->CreateComponent<CollisionShape>();
        // Set a box shape of size 1 x 1 x 1 for collision. The shape will be scaled with the scene node scale, so the
        // rendering and physics representation sizes should match (the box model is also 1 x 1 x 1.)
        shape->SetBox(Vector3::ONE);
    }

    // Create animated models
    for (int z = -1; z <= 1; ++z)
    {
        for (int x = -4; x <= 4; ++x)
        {
            Node* modelNode = scene_->CreateChild("Jack");
            modelNode->SetPosition(Vector3(x * 5.0f, 0.0f, z * 5.0f));
            modelNode->SetRotation(Quaternion(0.0f, 180.0f, 0.0f));
            auto* modelObject = modelNode->CreateComponent<AnimatedModel>();
            modelObject->SetModel(cache.GetResource<Model>("Models/Jack.mdl"));
            modelObject->SetMaterial(cache.GetResource<Material>("Materials/Jack.xml"));
            modelObject->SetCastShadows(true);
            // Set the model to also update when invisible to avoid staying invisible when the model should come into
            // view, but does not as the bounding box is not updated
            modelObject->SetUpdateInvisible(true);

            // Create a rigid body and a collision shape. These will act as a trigger for transforming the
            // model into a ragdoll when hit by a moving object
            auto* body = modelNode->CreateComponent<RigidBody>();
            // The Trigger mode makes the rigid body only detect collisions, but impart no forces on the
            // colliding objects
            body->SetTrigger(true);
            auto* shape = modelNode->CreateComponent<CollisionShape>();
            // Create the capsule shape with an offset so that it is correctly aligned with the model, which
            // has its origin at the feet
            shape->SetCapsule(0.7f, 2.0f, Vector3(0.0f, 1.0f, 0.0f));

            // Create a custom component that reacts to collisions and creates the ragdoll
            modelNode->CreateComponent<CreateRagdoll>();
        }
    }

    // Create the camera. Limit far clip distance to match the fog. Note: now we actually create the camera node outside
    // the scene, because we want it to be unaffected by scene load / save
    cameraNode_ = new Node();
    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(300.0f);

    // Set an initial position for the camera scene node above the floor
    cameraNode_->SetPosition(Vector3(0.0f, 3.0f, -20.0f));
}

void Ragdolls::CreateInstructions()
{
    // Construct new Text object, set string to display and font to use
    auto* instructionText = DV_UI.GetRoot()->CreateChild<Text>();
    instructionText->SetText(
        "Use WASD keys and mouse to move\n"
        "LMB to spawn physics objects\n"
        "F5 to save scene, F7 to load\n"
        "Space to toggle physics debug geometry"
    );
    instructionText->SetFont(DV_RES_CACHE.GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    // The text has multiple rows. Center them in relation to each other
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI.GetRoot()->GetHeight() / 4);
}

void Ragdolls::SetupViewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    DV_RENDERER.SetViewport(0, viewport);
}

void Ragdolls::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (DV_UI.GetFocusElement())
        return;

    Input& input = DV_INPUT;

    // Movement speed as world units per second
    const float MOVE_SPEED = 20.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    IntVector2 mouseMove = input.GetMouseMove();
    yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
    pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
    pitch_ = Clamp(pitch_, -90.0f, 90.0f);

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input.GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

    // "Shoot" a physics object with left mousebutton
    if (input.GetMouseButtonPress(MOUSEB_LEFT))
        SpawnObject();

    // Check for loading / saving the scene
    if (input.GetKeyPress(KEY_F5))
    {
        File saveFile(DV_FILE_SYSTEM.GetProgramDir() + "Data/Scenes/Ragdolls.xml", FILE_WRITE);
        scene_->save_xml(saveFile);
    }
    if (input.GetKeyPress(KEY_F7))
    {
        File loadFile(DV_FILE_SYSTEM.GetProgramDir() + "Data/Scenes/Ragdolls.xml", FILE_READ);
        scene_->load_xml(loadFile);
    }

    // Toggle physics debug geometry with space
    if (input.GetKeyPress(KEY_SPACE))
        drawDebug_ = !drawDebug_;
}

void Ragdolls::SpawnObject()
{
    Node* boxNode = scene_->CreateChild("Sphere");
    boxNode->SetPosition(cameraNode_->GetPosition());
    boxNode->SetRotation(cameraNode_->GetRotation());
    boxNode->SetScale(0.25f);
    auto* boxObject = boxNode->CreateComponent<StaticModel>();
    boxObject->SetModel(DV_RES_CACHE.GetResource<Model>("Models/Sphere.mdl"));
    boxObject->SetMaterial(DV_RES_CACHE.GetResource<Material>("Materials/StoneSmall.xml"));
    boxObject->SetCastShadows(true);

    auto* body = boxNode->CreateComponent<RigidBody>();
    body->SetMass(1.0f);
    body->SetRollingFriction(0.15f);
    auto* shape = boxNode->CreateComponent<CollisionShape>();
    shape->SetSphere(1.0f);

    const float OBJECT_VELOCITY = 10.0f;

    // Set initial velocity for the RigidBody based on camera forward vector. Add also a slight up component
    // to overcome gravity better
    body->SetLinearVelocity(cameraNode_->GetRotation() * Vector3(0.0f, 0.25f, 1.0f) * OBJECT_VELOCITY);
}

void Ragdolls::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, DV_HANDLER(Ragdolls, HandleUpdate));

    // Subscribe HandlePostRenderUpdate() function for processing the post-render update event, during which we request
    // debug geometry
    SubscribeToEvent(E_POSTRENDERUPDATE, DV_HANDLER(Ragdolls, HandlePostRenderUpdate));
}

void Ragdolls::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}

void Ragdolls::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    // If draw debug mode is enabled, draw physics debug geometry. Use depth test to make the result easier to interpret
    if (drawDebug_)
        scene_->GetComponent<PhysicsWorld>()->DrawDebugGeometry(true);
}
