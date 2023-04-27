// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/context.h>
#include <dviglo/input/input.h>
#include <dviglo/physics_2d/rigid_body_2d.h>
#include <dviglo/scene/scene.h>
#include <dviglo/scene/scene_events.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>
#include <dviglo/urho_2d/animated_sprite_2d.h>
#include <dviglo/urho_2d/animation_set_2d.h>

#include "character2d.h"

#include <dviglo/common/debug_new.h>

// Character2D logic component
Character2D::Character2D() :
    wounded_(false),
    killed_(false),
    timer_(0.0f),
    maxCoins_(0),
    remainingCoins_(0),
    remainingLifes_(3),
    moveSpeedScale_(1.0f),
    zoom_(0.0f)
{
}

void Character2D::register_object()
{
    DV_CONTEXT.RegisterFactory<Character2D>();

    // These macros register the class attributes to the Context for automatic load / save handling.
    // We specify the 'Default' attribute mode which means it will be used both for saving into file, and network replication.
    DV_ATTRIBUTE("Move Speed Scale", moveSpeedScale_, 1.0f, AM_DEFAULT);
    DV_ATTRIBUTE("Camera Zoom", zoom_, 0.0f, AM_DEFAULT);
    DV_ATTRIBUTE("Coins In Level", maxCoins_, 0, AM_DEFAULT);
    DV_ATTRIBUTE("Remaining Coins", remainingCoins_, 0, AM_DEFAULT);
    DV_ATTRIBUTE("Remaining Lifes", remainingLifes_, 3, AM_DEFAULT);
}

void Character2D::Update(float timeStep)
{
    // Handle wounded/killed states
    if (killed_)
        return;

    if (wounded_)
    {
        HandleWoundedState(timeStep);
        return;
    }

    auto* animatedSprite = GetComponent<AnimatedSprite2D>();
    Input& input = DV_INPUT;

    // Set direction
    Vector3 moveDir = Vector3::ZERO; // Reset
    float speedX = Clamp(MOVE_SPEED_X / zoom_, 0.4f, 1.0f);
    float speedY = speedX;

    // Используем скан-коды, а не коды клавиш, иначе не будет работать в Linux, когда включена русская раскладка клавиатуры
    if (input.GetScancodeDown(SCANCODE_A) || input.GetKeyDown(KEY_LEFT))
    {
        moveDir = moveDir + Vector3::LEFT * speedX;
        animatedSprite->SetFlipX(false); // Flip sprite (reset to default play on the X axis)
    }
    if (input.GetScancodeDown(SCANCODE_D) || input.GetKeyDown(KEY_RIGHT))
    {
        moveDir = moveDir + Vector3::RIGHT * speedX;
        animatedSprite->SetFlipX(true); // Flip sprite (flip animation on the X axis)
    }

    if (!moveDir.Equals(Vector3::ZERO))
        speedY = speedX * moveSpeedScale_;

    if (input.GetScancodeDown(SCANCODE_W) || input.GetKeyDown(KEY_UP))
        moveDir = moveDir + Vector3::UP * speedY;
    if (input.GetScancodeDown(SCANCODE_S) || input.GetKeyDown(KEY_DOWN))
        moveDir = moveDir + Vector3::DOWN * speedY;

    // Move
    if (!moveDir.Equals(Vector3::ZERO))
        node_->Translate(moveDir * timeStep);

    // Animate
    if (input.GetKeyDown(KEY_SPACE))
    {
        if (animatedSprite->GetAnimation() != "attack")
            animatedSprite->SetAnimation("attack", LM_FORCE_LOOPED);
    }
    else if (!moveDir.Equals(Vector3::ZERO))
    {
        if (animatedSprite->GetAnimation() != "run")
            animatedSprite->SetAnimation("run");
    }
    else if (animatedSprite->GetAnimation() != "idle")
    {
        animatedSprite->SetAnimation("idle");
    }
}

void Character2D::HandleWoundedState(float timeStep)
{
    auto* body = GetComponent<RigidBody2D>();
    auto* animatedSprite = GetComponent<AnimatedSprite2D>();

    // Play "hit" animation in loop
    if (animatedSprite->GetAnimation() != "hit")
        animatedSprite->SetAnimation("hit", LM_FORCE_LOOPED);

    // Update timer
    timer_ += timeStep;

    if (timer_ > 2.0f)
    {
        // Reset timer
        timer_ = 0.0f;

        // Clear forces (should be performed by setting linear velocity to zero, but currently doesn't work)
        body->SetLinearVelocity(Vector2::ZERO);
        body->SetAwake(false);
        body->SetAwake(true);

        // Remove particle emitter
        node_->GetChild("Emitter", true)->Remove();

        // Update lifes UI and counter
        remainingLifes_ -= 1;
        Text* lifeText = static_cast<Text*>(DV_UI->GetRoot()->GetChild("LifeText", true));
        lifeText->SetText(String(remainingLifes_)); // Update lifes UI counter

        // Reset wounded state
        wounded_ = false;

        // Handle death
        if (remainingLifes_ == 0)
        {
            HandleDeath();
            return;
        }

        // Re-position the character to the nearest point
        if (node_->GetPosition().x < 15.0f)
            node_->SetPosition(Vector3(-5.0f, 11.0f, 0.0f));
        else
            node_->SetPosition(Vector3(18.8f, 9.2f, 0.0f));
    }
}

void Character2D::HandleDeath()
{
    auto* body = GetComponent<RigidBody2D>();
    auto* animatedSprite = GetComponent<AnimatedSprite2D>();

    // Set state to 'killed'
    killed_ = true;

    // Update UI elements
    Text* instructions = static_cast<Text*>(DV_UI->GetRoot()->GetChild("Instructions", true));
    instructions->SetText("!!! GAME OVER !!!");
    static_cast<Text*>(DV_UI->GetRoot()->GetChild("ExitButton", true))->SetVisible(true);
    static_cast<Text*>(DV_UI->GetRoot()->GetChild("PlayButton", true))->SetVisible(true);

    // Show mouse cursor so that we can click
    DV_INPUT.SetMouseVisible(true);

    // Put character outside of the scene and magnify him
    node_->SetPosition(Vector3(-20.0f, 0.0f, 0.0f));
    node_->SetScale(1.2f);

    // Play death animation once
    if (animatedSprite->GetAnimation() != "dead")
        animatedSprite->SetAnimation("dead");
}
