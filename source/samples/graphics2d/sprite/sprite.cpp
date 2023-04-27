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
#include <dviglo/urho_2d/sprite_2d.h>

#include "sprite.h"

#include <dviglo/common/debug_new.h>

// Number of static sprites to draw
static const unsigned NUM_SPRITES = 200;
static const StringHash VAR_MOVESPEED("MoveSpeed");
static const StringHash VAR_ROTATESPEED("RotateSpeed");

DV_DEFINE_APPLICATION_MAIN(Urho2DSprite)

Urho2DSprite::Urho2DSprite()
{
}

void Urho2DSprite::Start()
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

void Urho2DSprite::create_scene()
{
    scene_ = new Scene();
    scene_->create_component<Octree>();

    // Create camera node
    cameraNode_ = scene_->create_child("Camera");
    // Set camera's position
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -10.0f));

    Camera* camera = cameraNode_->create_component<Camera>();
    camera->SetOrthographic(true);

    camera->SetOrthoSize((float)DV_GRAPHICS.GetHeight() * PIXEL_SIZE);

    // Get sprite
    Sprite2D* sprite = DV_RES_CACHE.GetResource<Sprite2D>("sprites/aster.png");
    if (!sprite)
        return;

    float halfWidth = DV_GRAPHICS.GetWidth() * 0.5f * PIXEL_SIZE;
    float halfHeight = DV_GRAPHICS.GetHeight() * 0.5f * PIXEL_SIZE;

    for (unsigned i = 0; i < NUM_SPRITES; ++i)
    {
        SharedPtr<Node> spriteNode(scene_->create_child("StaticSprite2D"));
        spriteNode->SetPosition(Vector3(Random(-halfWidth, halfWidth), Random(-halfHeight, halfHeight), 0.0f));

        StaticSprite2D* staticSprite = spriteNode->create_component<StaticSprite2D>();
        // Set random color
        staticSprite->SetColor(Color(Random(1.0f), Random(1.0f), Random(1.0f), 1.0f));
        // Set blend mode
        staticSprite->SetBlendMode(BLEND_ALPHA);
        // Set sprite
        staticSprite->SetSprite(sprite);

        // Set move speed
        spriteNode->SetVar(VAR_MOVESPEED, Vector3(Random(-2.0f, 2.0f), Random(-2.0f, 2.0f), 0.0f));
        // Set rotate speed
        spriteNode->SetVar(VAR_ROTATESPEED, Random(-90.0f, 90.0f));

        // Add to sprite node vector
        spriteNodes_.Push(spriteNode);
    }

    // Get animation set
    AnimationSet2D* animationSet = DV_RES_CACHE.GetResource<AnimationSet2D>("sprites/gold_icon.scml");
    if (!animationSet)
        return;

    SharedPtr<Node> spriteNode(scene_->create_child("AnimatedSprite2D"));
    spriteNode->SetPosition(Vector3(0.0f, 0.0f, -1.0f));

    AnimatedSprite2D* animatedSprite = spriteNode->create_component<AnimatedSprite2D>();
    // Set animation
    animatedSprite->SetAnimationSet(animationSet);
    animatedSprite->SetAnimation("idle");
}

void Urho2DSprite::create_instructions()
{
    // Construct new Text object, set string to display and font to use
    Text* instructionText = DV_UI->GetRoot()->create_child<Text>();
    instructionText->SetText("Use WASD keys to move, use PageUp PageDown keys to zoom.");
    instructionText->SetFont(DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI->GetRoot()->GetHeight() / 4);
}

void Urho2DSprite::setup_viewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    DV_RENDERER->SetViewport(0, viewport);
}

void Urho2DSprite::move_camera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (DV_UI->GetFocusElement())
        return;

    Input* input = DV_INPUT;

    // Movement speed as world units per second
    const float MOVE_SPEED = 4.0f;

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::UP * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::DOWN * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

    if (input->GetKeyDown(KEY_PAGEUP))
    {
        Camera* camera = cameraNode_->GetComponent<Camera>();
        camera->SetZoom(camera->GetZoom() * 1.01f);
    }

    if (input->GetKeyDown(KEY_PAGEDOWN))
    {
        Camera* camera = cameraNode_->GetComponent<Camera>();
        camera->SetZoom(camera->GetZoom() * 0.99f);
    }
}

void Urho2DSprite::subscribe_to_events()
{
    // Subscribe handle_update() function for processing update events
    subscribe_to_event(E_UPDATE, DV_HANDLER(Urho2DSprite, handle_update));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    unsubscribe_from_event(E_SCENEUPDATE);
}

void Urho2DSprite::handle_update(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    move_camera(timeStep);

    float halfWidth = (float)DV_GRAPHICS.GetWidth() * 0.5f * PIXEL_SIZE;
    float halfHeight = (float)DV_GRAPHICS.GetHeight() * 0.5f * PIXEL_SIZE;

    for (const SharedPtr<Node>& node : spriteNodes_)
    {
        Vector3 position = node->GetPosition();
        Vector3 moveSpeed = node->GetVar(VAR_MOVESPEED).GetVector3();
        Vector3 newPosition = position + moveSpeed * timeStep;
        if (newPosition.x < -halfWidth || newPosition.x > halfWidth)
        {
            newPosition.x = position.x;
            moveSpeed.x = -moveSpeed.x;
            node->SetVar(VAR_MOVESPEED, moveSpeed);
        }
        if (newPosition.y < -halfHeight || newPosition.y > halfHeight)
        {
            newPosition.y = position.y;
            moveSpeed.y = -moveSpeed.y;
            node->SetVar(VAR_MOVESPEED, moveSpeed);
        }

        node->SetPosition(newPosition);

        float rotateSpeed = node->GetVar(VAR_ROTATESPEED).GetFloat();
        node->Roll(rotateSpeed * timeStep);
    }
}
