// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/context.h>
#include <dviglo/input/input.h>
#include <dviglo/io/memory_buffer.h>
#include <dviglo/physics_2d/physics_world_2d.h>
#include <dviglo/physics_2d/rigid_body_2d.h>
#include <dviglo/scene/scene.h>
#include <dviglo/scene/scene_events.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>
#include <dviglo/urho_2d/animated_sprite_2d.h>
#include <dviglo/urho_2d/animation_set_2d.h>

#include <dviglo/common/debug_new.h>

#include "character2d.h"

// Character2D logic component
Character2D::Character2D() :
    wounded_(false),
    killed_(false),
    timer_(0.0f),
    maxCoins_(0),
    remainingCoins_(0),
    remainingLifes_(3),
    isClimbing_(false),
    climb2_(false),
    aboveClimbable_(false),
    onSlope_(false)
{
}

void Character2D::register_object()
{
    DV_CONTEXT.RegisterFactory<Character2D>();

    // These macros register the class attributes to the Context for automatic load / save handling.
    // We specify the 'Default' attribute mode which means it will be used both for saving into file, and network replication.
    DV_ATTRIBUTE("Wounded", wounded_, false, AM_DEFAULT);
    DV_ATTRIBUTE("Killed", killed_, false, AM_DEFAULT);
    DV_ATTRIBUTE("Timer", timer_, 0.0f, AM_DEFAULT);
    DV_ATTRIBUTE("Coins In Level", maxCoins_, 0, AM_DEFAULT);
    DV_ATTRIBUTE("Remaining Coins", remainingCoins_, 0, AM_DEFAULT);
    DV_ATTRIBUTE("Remaining Lifes", remainingLifes_, 3, AM_DEFAULT);
    // Note that we don't load/save isClimbing_ as the contact listener already sets this bool.
    DV_ATTRIBUTE("Is Climbing Rope", climb2_, false, AM_DEFAULT);
    DV_ATTRIBUTE("Is Above Climbable", aboveClimbable_, false, AM_DEFAULT);
    DV_ATTRIBUTE("Is On Slope", onSlope_, false, AM_DEFAULT);
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

    // Set temporary variables
    Input& input = DV_INPUT;
    auto* body = GetComponent<RigidBody2D>();
    auto* animatedSprite = GetComponent<AnimatedSprite2D>();
    bool onGround = false;
    bool jump = false;

    // Collision detection (AABB query)
    Vector2 characterHalfSize = Vector2(0.16f, 0.16f);
    auto* physicsWorld = GetScene()->GetComponent<PhysicsWorld2D>();
    Vector<RigidBody2D*> collidingBodies;
    physicsWorld->GetRigidBodies(collidingBodies, Rect(node_->GetWorldPosition2D() - characterHalfSize - Vector2(0.0f, 0.1f), node_->GetWorldPosition2D() + characterHalfSize));

    if (collidingBodies.Size() > 1 && !isClimbing_)
        onGround = true;

    // Set direction
    Vector2 moveDir = Vector2::ZERO; // Reset

    if (input.GetKeyDown(KEY_A) || input.GetKeyDown(KEY_LEFT))
    {
        moveDir = moveDir + Vector2::LEFT;
        animatedSprite->SetFlipX(false); // Flip sprite (reset to default play on the X axis)
    }
    if (input.GetKeyDown(KEY_D) || input.GetKeyDown(KEY_RIGHT))
    {
        moveDir = moveDir + Vector2::RIGHT;
        animatedSprite->SetFlipX(true); // Flip sprite (flip animation on the X axis)
    }

    // Jump
    if ((onGround || aboveClimbable_) && (input.GetKeyPress(KEY_W) || input.GetKeyPress(KEY_UP)))
        jump = true;

    // Climb
    if (isClimbing_)
    {
        if (!aboveClimbable_ && (input.GetKeyDown(KEY_UP) || input.GetKeyDown(KEY_W)))
            moveDir = moveDir + Vector2(0.0f, 1.0f);

        if (input.GetKeyDown(KEY_DOWN) || input.GetKeyDown(KEY_S))
            moveDir = moveDir + Vector2(0.0f, -1.0f);
    }

    // Move
    if (!moveDir.Equals(Vector2::ZERO) || jump)
    {
        if (onSlope_)
            body->ApplyForceToCenter(moveDir * MOVE_SPEED / 2, true); // When climbing a slope, apply force (todo: replace by setting linear velocity to zero when will work)
        else
            node_->Translate(Vector3(moveDir.x_, moveDir.y_, 0) * timeStep * 1.8f);
        if (jump)
            body->ApplyLinearImpulse(Vector2(0.0f, 0.17f) * MOVE_SPEED, body->GetMassCenter(), true);
    }

    // Animate
    if (input.GetKeyDown(KEY_SPACE))
    {
        if (animatedSprite->GetAnimation() != "attack")
        {
            animatedSprite->SetAnimation("attack", LM_FORCE_LOOPED);
            animatedSprite->SetSpeed(1.5f);
        }
    }
    else if (!moveDir.Equals(Vector2::ZERO))
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
        Text* lifeText = static_cast<Text*>(DV_UI.GetRoot()->GetChild("LifeText", true));
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
        if (node_->GetPosition().x_ < 15.0f)
            node_->SetPosition(Vector3(1.0f, 8.0f, 0.0f));
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
    Text* instructions = static_cast<Text*>(DV_UI.GetRoot()->GetChild("Instructions", true));
    instructions->SetText("!!! GAME OVER !!!");
    static_cast<Text*>(DV_UI.GetRoot()->GetChild("ExitButton", true))->SetVisible(true);
    static_cast<Text*>(DV_UI.GetRoot()->GetChild("PlayButton", true))->SetVisible(true);

    // Show mouse cursor so that we can click
    DV_INPUT.SetMouseVisible(true);

    // Put character outside of the scene and magnify him
    node_->SetPosition(Vector3(-20.0f, 0.0f, 0.0f));
    node_->SetScale(1.2f);

    // Play death animation once
    if (animatedSprite->GetAnimation() != "dead2")
        animatedSprite->SetAnimation("dead2");
}
