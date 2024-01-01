// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/input/input.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/urho_2d/sprite_2d.h>
#include <dviglo/urho_2d/static_sprite_2d.h>
#include <dviglo/urho_2d/stretchable_sprite_2d.h>

#include "stretchable_sprite.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(Urho2DStretchableSprite)

Urho2DStretchableSprite::Urho2DStretchableSprite()
{
}

void Urho2DStretchableSprite::Start()
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

void Urho2DStretchableSprite::create_scene()
{
    scene_ = new Scene();
    scene_->create_component<Octree>();

    // Create camera node
    cameraNode_ = scene_->create_child("Camera");
    // Set camera's position
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -10.0f));

    auto* camera = cameraNode_->create_component<Camera>();
    camera->SetOrthographic(true);

    camera->SetOrthoSize((float)DV_GRAPHICS->GetHeight() * PIXEL_SIZE);

    refSpriteNode_ = scene_->create_child("regular sprite");
    stretchSpriteNode_ = scene_->create_child("stretchable sprite");

    auto* sprite = DV_RES_CACHE->GetResource<Sprite2D>("sprites/stretchable.png");
    if (sprite)
    {
        refSpriteNode_->create_component<StaticSprite2D>()->SetSprite(sprite);

        auto stretchSprite = stretchSpriteNode_->create_component<StretchableSprite2D>();
        stretchSprite->SetSprite(sprite);
        stretchSprite->SetBorder({25, 25, 25, 25});

        refSpriteNode_->Translate2D(Vector2{-2.0f, 0.0f});
        stretchSpriteNode_->Translate2D(Vector2{2.0f, 0.0f});
    }
}

void Urho2DStretchableSprite::create_instructions()
{
    // Construct new Text object, set string to display and font to use
    auto* instructionText = DV_UI->GetRoot()->create_child<Text>();
    instructionText->SetText(
        "Use WASD keys to transform, Tab key to cycle through\n"
        "Scale, Rotate, and Translate transform modes. In Rotate\n"
        "mode, combine A/D keys with Ctrl key to rotate about\n"
        "the Z axis");
    instructionText->SetFont(DV_RES_CACHE->GetResource<Font>("fonts/anonymous pro.ttf"), 12);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI->GetRoot()->GetHeight() / 4);
}

void Urho2DStretchableSprite::setup_viewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    DV_RENDERER->SetViewport(0, viewport);
}

void Urho2DStretchableSprite::subscribe_to_events()
{
    subscribe_to_event(E_KEYUP, DV_HANDLER(Urho2DStretchableSprite, OnKeyUp));

    // Subscribe handle_update() function for processing update events
    subscribe_to_event(E_UPDATE, DV_HANDLER(Urho2DStretchableSprite, handle_update));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    unsubscribe_from_event(E_SCENEUPDATE);
}

void Urho2DStretchableSprite::handle_update(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    switch (selectTransform_)
    {
    case 0: ScaleSprites(timeStep);
        break;
    case 1: RotateSprites(timeStep);
        break;
    case 2: TranslateSprites(timeStep);
        break;
    default: DV_LOGERRORF("bad transform selection: %d", selectTransform_);
    }
}

void Urho2DStretchableSprite::OnKeyUp(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace KeyUp;

    const auto key = eventData[P_KEY].GetI32();

    if (key == KEY_TAB)
    {
        ++selectTransform_;
        selectTransform_ %= 3;
    }
    else if (key == KEY_ESCAPE)
        DV_ENGINE->Exit();
}


void Urho2DStretchableSprite::TranslateSprites(float timeStep)
{
    static constexpr auto speed = 1.0f;
    const auto left = DV_INPUT->GetKeyDown(KEY_A),
        right = DV_INPUT->GetKeyDown(KEY_D),
        up = DV_INPUT->GetKeyDown(KEY_W),
        down = DV_INPUT->GetKeyDown(KEY_S);

    if (left || right || up || down)
    {
        const auto quantum = timeStep * speed;
        const auto translate = Vector2{(left ? -quantum : 0.0f) + (right ? quantum : 0.0f),
                                       (down ? -quantum : 0.0f) + (up ? quantum : 0.0f)};

        refSpriteNode_->Translate2D(translate);
        stretchSpriteNode_->Translate2D(translate);
    }
}

void Urho2DStretchableSprite::RotateSprites(float timeStep)
{
    static constexpr auto speed = 45.0f;

    const auto left = DV_INPUT->GetKeyDown(KEY_A),
        right = DV_INPUT->GetKeyDown(KEY_D),
        up = DV_INPUT->GetKeyDown(KEY_W),
        down = DV_INPUT->GetKeyDown(KEY_S),
        ctrl = DV_INPUT->GetKeyDown(KEY_CTRL);

    if (left || right || up || down)
    {
        const auto quantum = timeStep * speed;

        const auto xrot = (up ? -quantum : 0.0f) + (down ? quantum : 0.0f);
        const auto rot2 = (left ? -quantum : 0.0f) + (right ? quantum : 0.0f);
        const auto totalRot = Quaternion{xrot, ctrl ? 0.0f : rot2, ctrl ? rot2 : 0.0f};

        refSpriteNode_->Rotate(totalRot);
        stretchSpriteNode_->Rotate(totalRot);
    }
}

void Urho2DStretchableSprite::ScaleSprites(float timeStep)
{
    static constexpr auto speed = 0.5f;

    const auto left = DV_INPUT->GetKeyDown(KEY_A),
        right = DV_INPUT->GetKeyDown(KEY_D),
        up = DV_INPUT->GetKeyDown(KEY_W),
        down = DV_INPUT->GetKeyDown(KEY_S);
    if (left || right || up || down)
    {
        const auto quantum = timeStep * speed;
        const auto scale = Vector2{1.0f + (right ? quantum : left ? -quantum : 0.0f),
                                   1.0f + (up ? quantum : down ? -quantum : 0.0f)};

        refSpriteNode_->Scale2D(scale);
        stretchSpriteNode_->Scale2D(scale);
    }
}
