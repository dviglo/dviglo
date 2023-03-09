// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/input/input.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/urho_2d/animated_sprite_2d.h>
#include <dviglo/urho_2d/animation_set_2d.h>

#include "spriter_animation.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(Urho2DSpriterAnimation)

Urho2DSpriterAnimation::Urho2DSpriterAnimation() :
    spriterAnimationIndex_(0)
{
}

void Urho2DSpriterAnimation::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);

    // Hook up to the frame update events
    SubscribeToEvents();
}

void Urho2DSpriterAnimation::CreateScene()
{
    scene_ = new Scene();
    scene_->CreateComponent<Octree>();

    // Create camera node
    cameraNode_ = scene_->CreateChild("Camera");
    // Set camera's position
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -10.0f));

    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetOrthographic(true);

    auto* graphics = GetSubsystem<Graphics>();
    camera->SetOrthoSize((float)graphics->GetHeight() * PIXEL_SIZE);
    camera->SetZoom(1.5f * Min((float)graphics->GetWidth() / 1280.0f, (float)graphics->GetHeight() / 800.0f)); // Set zoom according to user's resolution to ensure full visibility (initial zoom (1.5) is set for full visibility at 1280x800 resolution)

    auto* spriterAnimationSet = DV_RES_CACHE.GetResource<AnimationSet2D>("Urho2D/imp/imp.scml");
    if (!spriterAnimationSet)
        return;

    spriterNode_ = scene_->CreateChild("SpriterAnimation");
    auto* spriterAnimatedSprite = spriterNode_->CreateComponent<AnimatedSprite2D>();
    spriterAnimatedSprite->SetAnimationSet(spriterAnimationSet);
    spriterAnimatedSprite->SetAnimation(spriterAnimationSet->GetAnimation(spriterAnimationIndex_));
}

void Urho2DSpriterAnimation::CreateInstructions()
{
    // Construct new Text object, set string to display and font to use
    auto* instructionText = DV_UI.GetRoot()->CreateChild<Text>();
    instructionText->SetText("Mouse click to play next animation, \nUse WASD keys to move, use PageUp PageDown keys to zoom.");
    instructionText->SetFont(DV_RES_CACHE.GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    instructionText->SetTextAlignment(HA_CENTER); // Center rows in relation to each other

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI.GetRoot()->GetHeight() / 4);
}

void Urho2DSpriterAnimation::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void Urho2DSpriterAnimation::MoveCamera(float timeStep)
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

void Urho2DSpriterAnimation::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, DV_HANDLER(Urho2DSpriterAnimation, HandleUpdate));
    SubscribeToEvent(E_MOUSEBUTTONDOWN, DV_HANDLER(Urho2DSpriterAnimation, HandleMouseButtonDown));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    UnsubscribeFromEvent(E_SCENEUPDATE);
}

void Urho2DSpriterAnimation::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}

void Urho2DSpriterAnimation::HandleMouseButtonDown(StringHash eventType, VariantMap& eventData)
{
    auto* spriterAnimatedSprite = spriterNode_->GetComponent<AnimatedSprite2D>();
    AnimationSet2D* spriterAnimationSet = spriterAnimatedSprite->GetAnimationSet();
    spriterAnimationIndex_ = (spriterAnimationIndex_ + 1) % spriterAnimationSet->GetNumAnimations();
    spriterAnimatedSprite->SetAnimation(spriterAnimationSet->GetAnimation(spriterAnimationIndex_), LM_FORCE_LOOPED);
}
