// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/animated_model.h>
#include <dviglo/graphics/animation_controller.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/material.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/ribbon_trail.h>
#include <dviglo/input/input.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>
#include <dviglo/ui/text3d.h>

#include "ribbon_trail.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(RibbonTrailDemo)

RibbonTrailDemo::RibbonTrailDemo() :
    swordTrailStartTime_(0.2f),
    swordTrailEndTime_(0.46f),
    timeStepSum_(0.0f)
{
}

void RibbonTrailDemo::Start()
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
    Sample::InitMouseMode(MM_RELATIVE);
}

void RibbonTrailDemo::create_scene()
{
    ResourceCache& cache = DV_RES_CACHE;

    scene_ = new Scene();

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    scene_->create_component<Octree>();

    // Create scene node & StaticModel component for showing a static plane
    Node* planeNode = scene_->create_child("Plane");
    planeNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));
    auto* planeObject = planeNode->create_component<StaticModel>();
    planeObject->SetModel(cache.GetResource<Model>("models/plane.mdl"));
    planeObject->SetMaterial(cache.GetResource<Material>("materials/stone_tiled.xml"));

    // Create a directional light to the world.
    Node* lightNode = scene_->create_child("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f)); // The direction vector does not need to be normalized
    auto* light = lightNode->create_component<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00005f, 0.5f));
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));

    // Create first box for face camera trail demo with 1 column.
    boxNode1_ = scene_->create_child("Box1");
    auto* box1 = boxNode1_->create_component<StaticModel>();
    box1->SetModel(cache.GetResource<Model>("models/box.mdl"));
    box1->SetCastShadows(true);
    auto* boxTrail1 = boxNode1_->create_component<RibbonTrail>();
    boxTrail1->SetMaterial(cache.GetResource<Material>("materials/ribbon_trail.xml"));
    boxTrail1->SetStartColor(Color(1.0f, 0.5f, 0.0f, 1.0f));
    boxTrail1->SetEndColor(Color(1.0f, 1.0f, 0.0f, 0.0f));
    boxTrail1->SetWidth(0.5f);
    boxTrail1->SetUpdateInvisible(true);

    // Create second box for face camera trail demo with 4 column.
    // This will produce less distortion than first trail.
    boxNode2_ = scene_->create_child("Box2");
    auto* box2 = boxNode2_->create_component<StaticModel>();
    box2->SetModel(cache.GetResource<Model>("models/box.mdl"));
    box2->SetCastShadows(true);
    auto* boxTrail2 = boxNode2_->create_component<RibbonTrail>();
    boxTrail2->SetMaterial(cache.GetResource<Material>("materials/ribbon_trail.xml"));
    boxTrail2->SetStartColor(Color(1.0f, 0.5f, 0.0f, 1.0f));
    boxTrail2->SetEndColor(Color(1.0f, 1.0f, 0.0f, 0.0f));
    boxTrail2->SetWidth(0.5f);
    boxTrail2->SetTailColumn(4);
    boxTrail2->SetUpdateInvisible(true);

    // Load ninja animated model for bone trail demo.
    Node* ninjaNode = scene_->create_child("Ninja");
    ninjaNode->SetPosition(Vector3(5.0f, 0.0f, 0.0f));
    ninjaNode->SetRotation(Quaternion(0.0f, 180.0f, 0.0f));
    auto* ninja = ninjaNode->create_component<AnimatedModel>();
    ninja->SetModel(cache.GetResource<Model>("models/ninja_snow_war/ninja.mdl"));
    ninja->SetMaterial(cache.GetResource<Material>("materials/ninja_snow_war/ninja.xml"));
    ninja->SetCastShadows(true);

    // Create animation controller and play attack animation.
    ninjaAnimCtrl_ = ninjaNode->create_component<AnimationController>();
    ninjaAnimCtrl_->PlayExclusive("models/ninja_snow_war/ninja_attack3.ani", 0, true, 0.0f);

    // Add ribbon trail to tip of sword.
    Node* swordTip = ninjaNode->GetChild("Joint29", true);
    swordTrail_ = swordTip->create_component<RibbonTrail>();

    // Set sword trail type to bone and set other parameters.
    swordTrail_->SetTrailType(TT_BONE);
    swordTrail_->SetMaterial(cache.GetResource<Material>("materials/slash_trail.xml"));
    swordTrail_->SetLifetime(0.22f);
    swordTrail_->SetStartColor(Color(1.0f, 1.0f, 1.0f, 0.75f));
    swordTrail_->SetEndColor(Color(0.2f, 0.5f, 1.0f, 0.0f));
    swordTrail_->SetTailColumn(4);
    swordTrail_->SetUpdateInvisible(true);

    // Add floating text for info.
    Node* boxTextNode1 = scene_->create_child("BoxText1");
    boxTextNode1->SetPosition(Vector3(-1.0f, 2.0f, 0.0f));
    auto* boxText1 = boxTextNode1->create_component<Text3D>();
    boxText1->SetText(String("Face Camera Trail (4 Column)"));
    boxText1->SetFont(cache.GetResource<Font>("fonts/blue_highway.sdf"), 24);

    Node* boxTextNode2 = scene_->create_child("BoxText2");
    boxTextNode2->SetPosition(Vector3(-6.0f, 2.0f, 0.0f));
    auto* boxText2 = boxTextNode2->create_component<Text3D>();
    boxText2->SetText(String("Face Camera Trail (1 Column)"));
    boxText2->SetFont(cache.GetResource<Font>("fonts/blue_highway.sdf"), 24);

    Node* ninjaTextNode2 = scene_->create_child("NinjaText");
    ninjaTextNode2->SetPosition(Vector3(4.0f, 2.5f, 0.0f));
    auto* ninjaText = ninjaTextNode2->create_component<Text3D>();
    ninjaText->SetText(String("Bone Trail (4 Column)"));
    ninjaText->SetFont(cache.GetResource<Font>("fonts/blue_highway.sdf"), 24);

    // Create the camera.
    cameraNode_ = scene_->create_child("Camera");
    cameraNode_->create_component<Camera>();

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 2.0f, -14.0f));
}

void RibbonTrailDemo::create_instructions()
{
    // Construct new Text object, set string to display and font to use
    auto* instructionText = DV_UI->GetRoot()->create_child<Text>();
    instructionText->SetText("Use WASD keys and mouse to move");
    instructionText->SetFont(DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI->GetRoot()->GetHeight() / 4);
}

void RibbonTrailDemo::setup_viewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the camera
    // at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward / deferred) to
    // use, but now we just use full screen and default render path configured in the engine command line options
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    DV_RENDERER->SetViewport(0, viewport);
}

void RibbonTrailDemo::move_camera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (DV_UI->GetFocusElement())
        return;

    Input* input = DV_INPUT;

    // Movement speed as world units per second
    const float MOVE_SPEED = 20.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    IntVector2 mouseMove = input->GetMouseMove();
    yaw_ += MOUSE_SENSITIVITY * mouseMove.x;
    pitch_ += MOUSE_SENSITIVITY * mouseMove.y;
    pitch_ = Clamp(pitch_, -90.0f, 90.0f);

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    // Use the Translate() function (default local space) to move relative to the node's orientation.
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
}

void RibbonTrailDemo::subscribe_to_events()
{
    // Subscribe handle_update() function for processing update events
    subscribe_to_event(E_UPDATE, DV_HANDLER(RibbonTrailDemo, handle_update));
}

void RibbonTrailDemo::handle_update(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    move_camera(timeStep);

    // Sum of timesteps.
    timeStepSum_ += timeStep;

    // Move first box with pattern.
    boxNode1_->SetTransform(Vector3(-4.0f + 3.0f * Cos(100.0f * timeStepSum_), 0.5f, -2.0f * Cos(400.0f * timeStepSum_)),
        Quaternion());

    // Move second box with pattern.
    boxNode2_->SetTransform(Vector3(0.0f + 3.0f * Cos(100.0f * timeStepSum_), 0.5f, -2.0f * Cos(400.0f * timeStepSum_)),
        Quaternion());

    // Get elapsed attack animation time.
    float swordAnimTime = ninjaAnimCtrl_->GetAnimationState(String("models/ninja_snow_war/ninja_attack3.ani"))->GetTime();

    // Stop emitting trail when sword is finished slashing.
    if (!swordTrail_->IsEmitting() && swordAnimTime > swordTrailStartTime_ && swordAnimTime < swordTrailEndTime_)
        swordTrail_->SetEmitting(true);
    else if (swordTrail_->IsEmitting() && swordAnimTime >= swordTrailEndTime_)
        swordTrail_->SetEmitting(false);
}
