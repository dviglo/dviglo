// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/core/process_utils.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/animated_model.h>
#include <dviglo/graphics/animation_controller.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/light.h>
#include <dviglo/graphics/material.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/input/controls.h>
#include <dviglo/input/input.h>
#include <dviglo/io/file_system.h>
#include <dviglo/physics/collision_shape.h>
#include <dviglo/physics/physics_world.h>
#include <dviglo/physics/rigid_body.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

#include "character.h"
#include "demo.h"

#include <dviglo/common/debug_new.h>

constexpr float CAMERA_MIN_DIST = 1.0f;
constexpr float CAMERA_INITIAL_DIST = 5.0f;
constexpr float CAMERA_MAX_DIST = 20.0f;

DV_DEFINE_APPLICATION_MAIN(CharacterDemo)

CharacterDemo::CharacterDemo() :
    firstPerson_(false)
{
    // Register factory and attributes for the Character component so it can be created via create_component, and loaded / saved
    Character::register_object();
}

CharacterDemo::~CharacterDemo() = default;

void CharacterDemo::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create static scene content
    create_scene();

    // Create the controllable character
    CreateCharacter();

    // Create the UI content
    create_instructions();

    // Subscribe to necessary events
    subscribe_to_events();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void CharacterDemo::create_scene()
{
    ResourceCache& cache = DV_RES_CACHE;

    scene_ = new Scene();

    // Create scene subsystem components
    scene_->create_component<Octree>();
    scene_->create_component<PhysicsWorld>();

    // Create camera and define viewport. We will be doing load / save, so it's convenient to create the camera outside the scene,
    // so that it won't be destroyed and recreated, and we don't have to redefine the viewport on load
    cameraNode_ = new Node();
    auto* camera = cameraNode_->create_component<Camera>();
    camera->SetFarClip(300.0f);
    DV_RENDERER.SetViewport(0, new Viewport(scene_, camera));

    // Create static scene content. First create a zone for ambient lighting and fog control
    Node* zoneNode = scene_->create_child("Zone");
    auto* zone = zoneNode->create_component<Zone>();
    zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
    zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));

    // Create a directional light with cascaded shadow mapping
    Node* lightNode = scene_->create_child("DirectionalLight");
    lightNode->SetDirection(Vector3(0.3f, -0.5f, 0.425f));
    auto* light = lightNode->create_component<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
    light->SetSpecularIntensity(0.5f);

    // Create the floor object
    Node* floorNode = scene_->create_child("Floor");
    floorNode->SetPosition(Vector3(0.0f, -0.5f, 0.0f));
    floorNode->SetScale(Vector3(200.0f, 1.0f, 200.0f));
    auto* object = floorNode->create_component<StaticModel>();
    object->SetModel(cache.GetResource<Model>("models/box.mdl"));
    object->SetMaterial(cache.GetResource<Material>("materials/stone.xml"));

    auto* body = floorNode->create_component<RigidBody>();
    // Use collision layer bit 2 to mark world scenery. This is what we will raycast against to prevent camera from going
    // inside geometry
    body->SetCollisionLayer(2);
    auto* shape = floorNode->create_component<CollisionShape>();
    shape->SetBox(Vector3::ONE);

    // Create mushrooms of varying sizes
    const unsigned NUM_MUSHROOMS = 60;
    for (unsigned i = 0; i < NUM_MUSHROOMS; ++i)
    {
        Node* objectNode = scene_->create_child("Mushroom");
        objectNode->SetPosition(Vector3(Random(180.0f) - 90.0f, 0.0f, Random(180.0f) - 90.0f));
        objectNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
        objectNode->SetScale(2.0f + Random(5.0f));
        auto* object = objectNode->create_component<StaticModel>();
        object->SetModel(cache.GetResource<Model>("models/mushroom.mdl"));
        object->SetMaterial(cache.GetResource<Material>("materials/mushroom.xml"));
        object->SetCastShadows(true);

        auto* body = objectNode->create_component<RigidBody>();
        body->SetCollisionLayer(2);
        auto* shape = objectNode->create_component<CollisionShape>();
        shape->SetTriangleMesh(object->GetModel(), 0);
    }

    // Create movable boxes. Let them fall from the sky at first
    const unsigned NUM_BOXES = 100;
    for (unsigned i = 0; i < NUM_BOXES; ++i)
    {
        float scale = Random(2.0f) + 0.5f;

        Node* objectNode = scene_->create_child("Box");
        objectNode->SetPosition(Vector3(Random(180.0f) - 90.0f, Random(10.0f) + 10.0f, Random(180.0f) - 90.0f));
        objectNode->SetRotation(Quaternion(Random(360.0f), Random(360.0f), Random(360.0f)));
        objectNode->SetScale(scale);
        auto* object = objectNode->create_component<StaticModel>();
        object->SetModel(cache.GetResource<Model>("models/box.mdl"));
        object->SetMaterial(cache.GetResource<Material>("materials/stone.xml"));
        object->SetCastShadows(true);

        auto* body = objectNode->create_component<RigidBody>();
        body->SetCollisionLayer(2);
        // Bigger boxes will be heavier and harder to move
        body->SetMass(scale * 2.0f);
        auto* shape = objectNode->create_component<CollisionShape>();
        shape->SetBox(Vector3::ONE);
    }
}

void CharacterDemo::CreateCharacter()
{
    Node* objectNode = scene_->create_child("Jack");
    objectNode->SetPosition(Vector3(0.0f, 1.0f, 0.0f));

    // spin node
    Node* adjustNode = objectNode->create_child("AdjNode");
    adjustNode->SetRotation( Quaternion(180, Vector3(0,1,0) ) );

    // Create the rendering component + animation controller
    auto* object = adjustNode->create_component<AnimatedModel>();
    object->SetModel(DV_RES_CACHE.GetResource<Model>("models/Mutant/Mutant.mdl"));
    object->SetMaterial(DV_RES_CACHE.GetResource<Material>("models/Mutant/materials/mutant_m.xml"));
    object->SetCastShadows(true);
    adjustNode->create_component<AnimationController>();

    // Set the head bone for manual control
    object->GetSkeleton().GetBone("Mutant:Head")->animated_ = false;

    // Create rigidbody, and set non-zero mass so that the body becomes dynamic
    auto* body = objectNode->create_component<RigidBody>();
    body->SetCollisionLayer(1);
    body->SetMass(1.0f);

    // Set zero angular factor so that physics doesn't turn the character on its own.
    // Instead we will control the character yaw manually
    body->SetAngularFactor(Vector3::ZERO);

    // Set the rigidbody to signal collision also when in rest, so that we get ground collisions properly
    body->SetCollisionEventMode(COLLISION_ALWAYS);

    // Set a capsule shape for collision
    auto* shape = objectNode->create_component<CollisionShape>();
    shape->SetCapsule(0.7f, 1.8f, Vector3(0.0f, 0.9f, 0.0f));

    // Create the character logic component, which takes care of steering the rigidbody
    // Remember it so that we can set the controls. Use a WeakPtr because the scene hierarchy already owns it
    // and keeps it alive as long as it's not removed from the hierarchy
    character_ = objectNode->create_component<Character>();
}

void CharacterDemo::create_instructions()
{
    // Construct new Text object, set string to display and font to use
    auto* instructionText = DV_UI.GetRoot()->create_child<Text>();
    instructionText->SetText(
        "Use WASD keys and mouse to move\n"
        "Space to jump, F to toggle 1st/3rd person\n"
        "F5 to save scene, F7 to load"
    );
    instructionText->SetFont(DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf"), 15);
    // The text has multiple rows. Center them in relation to each other
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI.GetRoot()->GetHeight() / 4);
}

void CharacterDemo::subscribe_to_events()
{
    // Subscribe to Update event for setting the character controls before physics simulation
    subscribe_to_event(E_UPDATE, DV_HANDLER(CharacterDemo, handle_update));

    // Subscribe to PostUpdate event for updating the camera position after physics simulation
    subscribe_to_event(E_POSTUPDATE, DV_HANDLER(CharacterDemo, HandlePostUpdate));

    // Unsubscribe the SceneUpdate event from base class as the camera node is being controlled in HandlePostUpdate() in this sample
    unsubscribe_from_event(E_SCENEUPDATE);
}

void CharacterDemo::handle_update(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    Input& input = DV_INPUT;

    if (character_)
    {
        // Clear previous controls
        character_->controls_.Set(CTRL_FORWARD | CTRL_BACK | CTRL_LEFT | CTRL_RIGHT | CTRL_JUMP, false);

        // Update controls using keys
        if (!DV_UI.GetFocusElement())
        {
            // Используем скан-коды, а не коды клавиш, иначе не будет работать в Linux, когда включена русская раскладка клавиатуры
            character_->controls_.Set(CTRL_FORWARD, input.GetScancodeDown(SCANCODE_W));
            character_->controls_.Set(CTRL_BACK, input.GetScancodeDown(SCANCODE_S));
            character_->controls_.Set(CTRL_LEFT, input.GetScancodeDown(SCANCODE_A));
            character_->controls_.Set(CTRL_RIGHT, input.GetScancodeDown(SCANCODE_D));
            character_->controls_.Set(CTRL_JUMP, input.GetScancodeDown(SCANCODE_SPACE));

            // Add character yaw & pitch from the mouse motion
            character_->controls_.yaw_ += (float)input.GetMouseMoveX() * YAW_SENSITIVITY;
            character_->controls_.pitch_ += (float)input.GetMouseMoveY() * YAW_SENSITIVITY;
            // Limit pitch
            character_->controls_.pitch_ = Clamp(character_->controls_.pitch_, -80.0f, 80.0f);
            // Set rotation already here so that it's updated every rendering frame instead of every physics frame
            character_->GetNode()->SetRotation(Quaternion(character_->controls_.yaw_, Vector3::UP));

            // Switch between 1st and 3rd person
            if (input.GetKeyPress(KEY_F))
                firstPerson_ = !firstPerson_;

            // Check for loading / saving the scene
            if (input.GetKeyPress(KEY_F5))
            {
                File saveFile(DV_FILE_SYSTEM.GetProgramDir() + "data/scenes/character_demo.xml", FILE_WRITE);
                scene_->save_xml(saveFile);
            }
            if (input.GetKeyPress(KEY_F7))
            {
                File loadFile(DV_FILE_SYSTEM.GetProgramDir() + "data/scenes/character_demo.xml", FILE_READ);
                scene_->load_xml(loadFile);
                // After loading we have to reacquire the weak pointer to the Character component, as it has been recreated
                // Simply find the character's scene node by name as there's only one of them
                Node* characterNode = scene_->GetChild("Jack", true);
                if (characterNode)
                    character_ = characterNode->GetComponent<Character>();
            }
        }
    }
}

void CharacterDemo::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    if (!character_)
        return;

    Node* characterNode = character_->GetNode();

    // Get camera lookat dir from character yaw + pitch
    const Quaternion& rot = characterNode->GetRotation();
    Quaternion dir = rot * Quaternion(character_->controls_.pitch_, Vector3::RIGHT);

    // Turn head to camera pitch, but limit to avoid unnatural animation
    Node* headNode = characterNode->GetChild("Mutant:Head", true);
    float limitPitch = Clamp(character_->controls_.pitch_, -45.0f, 45.0f);
    Quaternion headDir = rot * Quaternion(limitPitch, Vector3(1.0f, 0.0f, 0.0f));
    // This could be expanded to look at an arbitrary target, now just look at a point in front
    Vector3 headWorldTarget = headNode->GetWorldPosition() + headDir * Vector3(0.0f, 0.0f, -1.0f);
    headNode->LookAt(headWorldTarget, Vector3(0.0f, 1.0f, 0.0f));

    if (firstPerson_)
    {
        cameraNode_->SetPosition(headNode->GetWorldPosition() + rot * Vector3(0.0f, 0.15f, 0.2f));
        cameraNode_->SetRotation(dir);
    }
    else
    {
        // Third person camera: position behind the character
        Vector3 aimPoint = characterNode->GetPosition() + rot * Vector3(0.0f, 1.7f, 0.0f);

        // Collide camera ray with static physics objects (layer bitmask 2) to ensure we see the character properly
        Vector3 rayDir = dir * Vector3::BACK;
        float rayDistance = CAMERA_INITIAL_DIST;
        PhysicsRaycastResult result;
        scene_->GetComponent<PhysicsWorld>()->RaycastSingle(result, Ray(aimPoint, rayDir), rayDistance, 2);
        if (result.body_)
            rayDistance = Min(rayDistance, result.distance_);
        rayDistance = Clamp(rayDistance, CAMERA_MIN_DIST, CAMERA_MAX_DIST);

        cameraNode_->SetPosition(aimPoint + rayDir * rayDistance);
        cameraNode_->SetRotation(dir);
    }
}
