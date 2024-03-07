// Copyright (c) the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#include "ninja.h"

#include "snowball.h"
#include "utilities/spawn.h"

static constexpr i32 LAYER_MOVE = 0;
static constexpr i32 LAYER_ATTACK = 1;

static constexpr float NINJA_MOVE_FORCE = 25.f;
static constexpr float NINJA_AIR_MOVE_FORCE = 1.f;
static constexpr float NINJA_DAMPING_FORCE = 5.f;
static constexpr float NINJA_JUMP_FORCE = 450.f;
static const Vector3 NINJA_THROW_VELOCITY(0.f, 4.25f, 20.f);
static const Vector3 NINJA_THROW_POSITION(0.f, 0.2f, 1.f);
static constexpr float NINJA_THROW_DELAY = 0.1f;
static constexpr float NINJA_CORPSE_DURATION = 3.f;
static constexpr i32 NINJA_POINTS = 250;

void Ninja::register_object()
{
    DV_CONTEXT->RegisterFactory<Ninja>();
}

Ninja::Ninja()
    : okToJump(false)
    , smoke(false)
    , inAirTime(1.f)
    , onGroundTime(0.f)
    , throwTime(0.f)
    , deathTime(0.f)
    , deathDir(0.f)
    , dirChangeTime(0.f)
    , aimX(0.f)
    , aimY(0.f)
{
    health = maxHealth = 2;
    onGround = false;
    isSliding = false;
}

void Ninja::DelayedStart()
{
    subscribe_to_event(node_, E_NODECOLLISION, DV_HANDLER(Ninja, HandleNodeCollision));

    // Get horizontal aim from initial rotation
    aimX = controls.yaw_ = node_->GetRotation().YawAngle();

    // Start playing the idle animation immediately, even before the first physics update
    AnimationController* animCtrl = node_->GetChild(0)->GetComponent<AnimationController>();
    animCtrl->PlayExclusive("models/ninja_snow_war/ninja_idle3.ani", LAYER_MOVE, true);
}

void Ninja::SetControls(const Controls& newControls)
{
    controls = newControls;
}

Quaternion Ninja::GetAim()
{
    Quaternion q = Quaternion(aimX, Vector3(0.f, 1.f, 0.f));
    q = q * Quaternion(aimY, Vector3(1.f, 0.f, 0.f));
    return q;
}

void Ninja::FixedUpdate(float timeStep)
{
    // For multiplayer, replicate the health into the node user variables
    node_->SetVar("Health", health);

    if (health <= 0)
    {
        DeathUpdate(timeStep);
        return;
    }

    // AI control if controller exists
    if (controller)
        controller->Control(this, node_, timeStep);

    RigidBody* body = node_->GetComponent<RigidBody>();
    AnimationController* animCtrl = node_->GetChild(0)->GetComponent<AnimationController>();

    // Turning / horizontal aiming
    if (aimX != controls.yaw_)
        aimX = controls.yaw_;

    // Vertical aiming
    if (aimY != controls.pitch_)
        aimY = controls.pitch_;

    // Force the physics rotation
    Quaternion q(aimX, Vector3(0.f, 1.f, 0.f));
    body->SetRotation(q);

    // Movement ground/air
    Vector3 vel = body->GetLinearVelocity();
    if (onGround)
    {
        // If landed, play a particle effect at feet (use the AnimatedModel node)
        if (inAirTime > 0.5)
            SpawnParticleEffect(node_->GetScene(), node_->GetChild(0)->GetWorldPosition(), "particle/snow_explosion.xml", 1);

        inAirTime = 0;
        onGroundTime += timeStep;
    }
    else
    {
        onGroundTime = 0;
        inAirTime += timeStep;
    }

    if (inAirTime < 0.3f && !isSliding)
    {
        bool sideMove = false;

        // Movement in four directions
        if (controls.IsDown(CTRL_UP | CTRL_DOWN | CTRL_LEFT | CTRL_RIGHT))
        {
            float animDir = 1.0f;
            Vector3 force(0, 0, 0);
            if (controls.IsDown(CTRL_UP))
                force += q * Vector3(0, 0, 1);
            if (controls.IsDown(CTRL_DOWN))
            {
                animDir = -1.0f;
                force += q * Vector3(0, 0, -1);
            }
            if (controls.IsDown(CTRL_LEFT))
            {
                sideMove = true;
                force += q * Vector3(-1, 0, 0);
            }
            if (controls.IsDown(CTRL_RIGHT))
            {
                sideMove = true;
                force += q * Vector3(1, 0, 0);
            }
            // Normalize so that diagonal strafing isn't faster
            force.Normalize();
            force *= NINJA_MOVE_FORCE;
            body->ApplyImpulse(force);

            // Walk or sidestep animation
            if (sideMove)
            {
                animCtrl->PlayExclusive("models/ninja_snow_war/ninja_stealth.ani", LAYER_MOVE, true, 0.2f);
                animCtrl->SetSpeed("models/ninja_snow_war/ninja_stealth.ani", animDir * 2.2f);
            }
            else
            {
                animCtrl->PlayExclusive("models/ninja_snow_war/ninja_walk.ani", LAYER_MOVE, true, 0.2f);
                animCtrl->SetSpeed("models/ninja_snow_war/ninja_walk.ani", animDir * 1.6f);
            }
        }
        else
        {
            // Idle animation
            animCtrl->PlayExclusive("models/ninja_snow_war/ninja_idle3.ani", LAYER_MOVE, true, 0.2f);
        }

        // Overall damping to cap maximum speed
        body->ApplyImpulse(Vector3(-NINJA_DAMPING_FORCE * vel.x, 0, -NINJA_DAMPING_FORCE * vel.z));

        // Jumping
        if (controls.IsDown(CTRL_JUMP))
        {
            if (okToJump && inAirTime < 0.1f)
            {
                // Lift slightly off the ground for better animation
                body->SetPosition(body->GetPosition() + Vector3(0.f, 0.03f, 0.f));
                body->ApplyImpulse(Vector3(0.f, NINJA_JUMP_FORCE, 0.f));
                inAirTime = 1.0f;
                animCtrl->PlayExclusive("models/ninja_snow_war/ninja_jump_no_height.ani", LAYER_MOVE, false, 0.1f);
                animCtrl->SetTime("models/ninja_snow_war/ninja_jump_no_height.ani", 0.0f); // Always play from beginning
                okToJump = false;
            }
        }
        else okToJump = true;
    }
    else
    {
        // Motion in the air
        // Note: when sliding a steep slope, control (or damping) isn't allowed!
        if (inAirTime > 0.3f && !isSliding)
        {
            if (controls.IsDown(CTRL_UP | CTRL_DOWN | CTRL_LEFT | CTRL_RIGHT))
            {
                Vector3 force(0, 0, 0);
                if (controls.IsDown(CTRL_UP))
                    force += q * Vector3(0, 0, 1);
                if (controls.IsDown(CTRL_DOWN))
                    force += q * Vector3(0, 0, -1);
                if (controls.IsDown(CTRL_LEFT))
                    force += q * Vector3(-1, 0, 0);
                if (controls.IsDown(CTRL_RIGHT))
                    force += q * Vector3(1, 0, 0);
                // Normalize so that diagonal strafing isn't faster
                force.Normalize();
                force *= NINJA_AIR_MOVE_FORCE;
                body->ApplyImpulse(force);
            }
        }

        // Falling/jumping/sliding animation
        if (inAirTime > 0.1f)
            animCtrl->PlayExclusive("models/ninja_snow_war/ninja_jump_no_height.ani", LAYER_MOVE, false, 0.1f);
    }

    // Shooting
    if (throwTime >= 0)
        throwTime -= timeStep;

    // Start fading the attack animation after it has progressed past a certain point
    if (animCtrl->GetTime("models/ninja_snow_war/ninja_attack1.ani") > 0.1f)
        animCtrl->Fade("models/ninja_snow_war/ninja_attack1.ani", 0.0f, 0.5f);

    if (controls.IsPressed(CTRL_FIRE, prevControls) && throwTime <= 0.f)
    {
        Vector3 projectileVel = GetAim() * NINJA_THROW_VELOCITY;

        animCtrl->Play("models/ninja_snow_war/ninja_attack1.ani", LAYER_ATTACK, false, 0.0f);
        animCtrl->SetTime("models/ninja_snow_war/ninja_attack1.ani", 0.0f); // Always play from beginning

        Node* snowball = SpawnObject(node_->GetScene(), node_->GetPosition() + vel * timeStep + q * NINJA_THROW_POSITION, GetAim(), "snowball");
        RigidBody* snowballBody = snowball->GetComponent<RigidBody>();
        snowballBody->SetLinearVelocity(projectileVel);
        Snowball* snowballObject = snowball->GetComponent<Snowball>();
        snowballObject->side = side;
        snowballObject->creatorID = node_->GetID();

        PlaySound("sounds/nut_throw.wav");

        throwTime = NINJA_THROW_DELAY;
    }

    prevControls = controls;

    ResetWorldCollision();
}

void Ninja::DeathUpdate(float timeStep)
{
    RigidBody* body = node_->GetComponent<RigidBody>();
    CollisionShape* shape = node_->GetComponent<CollisionShape>();
    Node* modelNode = node_->GetChild(0);
    AnimationController* animCtrl = modelNode->GetComponent<AnimationController>();
    AnimatedModel* model = modelNode->GetComponent<AnimatedModel>();

    Vector3 vel = body->GetLinearVelocity();

    // Overall damping to cap maximum speed
    body->ApplyImpulse(Vector3(-NINJA_DAMPING_FORCE * vel.x, 0, -NINJA_DAMPING_FORCE * vel.z));

    // Collide only to world geometry
    body->SetCollisionMask(2);

    // Pick death animation on first death update
    if (deathDir == 0)
    {
        if (Random(1.0f) < 0.5f)
            deathDir = -1.f;
        else
            deathDir = 1.f;

        PlaySound("sounds/small_explosion.wav");

        VariantMap eventData;
        eventData["Points"] = NINJA_POINTS;
        eventData["Receiver"] = lastDamageCreatorID;
        eventData["DamageSide"] = lastDamageSide;
        SendEvent("Points", eventData);
        SendEvent("Kill", eventData);
    }

    deathTime += timeStep;

    // Move the model node to center the corpse mostly within the physics cylinder
    // (because of the animation)
    if (deathDir < 0.f)
    {
        // Backward death
        animCtrl->StopLayer(LAYER_ATTACK, 0.1f);
        animCtrl->PlayExclusive("models/ninja_snow_war/ninja_death1.ani", LAYER_MOVE, false, 0.2f);
        animCtrl->SetSpeed("models/ninja_snow_war/ninja_death1.ani", 0.5f);
        if (deathTime >= 0.3f && deathTime < 0.8f)
            modelNode->Translate(Vector3(0.f, 0.f, 4.25f * timeStep));
    }
    else if (deathDir > 0.f)
    {
        // Forward death
        animCtrl->StopLayer(LAYER_ATTACK, 0.1f);
        animCtrl->PlayExclusive("models/ninja_snow_war/ninja_death2.ani", LAYER_MOVE, false, 0.2f);
        animCtrl->SetSpeed("models/ninja_snow_war/ninja_death2.ani", 0.5f);
        if (deathTime >= 0.4f && deathTime < 0.8f)
            modelNode->Translate(Vector3(0.f, 0.f, -4.25f * timeStep));
    }

    // Create smokecloud just before vanishing
    if (deathTime > NINJA_CORPSE_DURATION - 1.f && !smoke)
    {
        SpawnParticleEffect(node_->GetScene(), node_->GetPosition() + Vector3(0.f, -0.4f, 0.f), "particle/smoke.xml", 8.f);
        smoke = true;
    }

    if (deathTime > NINJA_CORPSE_DURATION)
    {
        SpawnObject(node_->GetScene(), node_->GetPosition() + Vector3(0.f, -0.5f, 0.f), Quaternion(), "light_flash");
        SpawnSound(node_->GetScene(), node_->GetPosition() + Vector3(0.f, -0.5f, 0.f), "sounds/big_explosion.wav", 2.f);
        node_->Remove();
    }
}

bool Ninja::Heal(i32 amount)
{
    if (health == maxHealth)
        return false;

    health += amount;
    if (health > maxHealth)
        health = maxHealth;
    // If player, play the "powerup" sound
    if (side == SIDE_PLAYER)
        PlaySound("sounds/powerup.wav");

    return true;
}
