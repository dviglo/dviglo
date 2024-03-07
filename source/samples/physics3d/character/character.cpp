// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include <dviglo/core/context.h>
#include <dviglo/graphics/animation_controller.h>
#include <dviglo/io/memory_buffer.h>
#include <dviglo/physics/physics_events.h>
#include <dviglo/physics/physics_world.h>
#include <dviglo/physics/rigid_body.h>
#include <dviglo/scene/scene.h>
#include <dviglo/scene/scene_events.h>

#include "character.h"

Character::Character() :
    onGround_(false),
    okToJump_(true),
    inAirTimer_(0.0f)
{
    // Only the physics update event is needed: unsubscribe from the rest for optimization
    SetUpdateEventMask(LogicComponentEvents::FixedUpdate);
}

void Character::register_object()
{
    DV_CONTEXT->RegisterFactory<Character>();

    // These macros register the class attributes to the Context for automatic load / save handling.
    // We specify the Default attribute mode which means it will be used both for saving into file, and network replication
    DV_ATTRIBUTE("Controls Yaw", controls_.yaw_, 0.0f, AM_DEFAULT);
    DV_ATTRIBUTE("Controls Pitch", controls_.pitch_, 0.0f, AM_DEFAULT);
    DV_ATTRIBUTE("On Ground", onGround_, false, AM_DEFAULT);
    DV_ATTRIBUTE("OK To Jump", okToJump_, true, AM_DEFAULT);
    DV_ATTRIBUTE("In Air Timer", inAirTimer_, 0.0f, AM_DEFAULT);
}

void Character::Start()
{
    // Component has been inserted into its scene node. Subscribe to events now
    subscribe_to_event(GetNode(), E_NODECOLLISION, DV_HANDLER(Character, HandleNodeCollision));
}

void Character::FixedUpdate(float timeStep)
{
    /// \todo Could cache the components for faster access instead of finding them each frame
    auto* body = GetComponent<RigidBody>();
    auto* animCtrl = node_->GetComponent<AnimationController>(true);

    // Update the in air timer. Reset if grounded
    if (!onGround_)
        inAirTimer_ += timeStep;
    else
        inAirTimer_ = 0.0f;
    // When character has been in air less than 1/10 second, it's still interpreted as being on ground
    bool softGrounded = inAirTimer_ < INAIR_THRESHOLD_TIME;

    // Update movement & animation
    const Quaternion& rot = node_->GetRotation();
    Vector3 moveDir = Vector3::ZERO;
    const Vector3& velocity = body->GetLinearVelocity();
    // Velocity on the XZ plane
    Vector3 planeVelocity(velocity.x, 0.0f, velocity.z);

    if (controls_.IsDown(CTRL_FORWARD))
        moveDir += Vector3::FORWARD;
    if (controls_.IsDown(CTRL_BACK))
        moveDir += Vector3::BACK;
    if (controls_.IsDown(CTRL_LEFT))
        moveDir += Vector3::LEFT;
    if (controls_.IsDown(CTRL_RIGHT))
        moveDir += Vector3::RIGHT;

    // Normalize move vector so that diagonal strafing is not faster
    if (moveDir.LengthSquared() > 0.0f)
        moveDir.Normalize();

    // If in air, allow control, but slower than when on ground
    body->ApplyImpulse(rot * moveDir * (softGrounded ? MOVE_FORCE : INAIR_MOVE_FORCE));

    if (softGrounded)
    {
        // When on ground, apply a braking force to limit maximum ground velocity
        Vector3 brakeForce = -planeVelocity * BRAKE_FORCE;
        body->ApplyImpulse(brakeForce);

        // Jump. Must release jump control between jumps
        if (controls_.IsDown(CTRL_JUMP))
        {
            if (okToJump_)
            {
                body->ApplyImpulse(Vector3::UP * JUMP_FORCE);
                okToJump_ = false;
                animCtrl->PlayExclusive("models/mutant/mutant_jump1.ani", 0, false, 0.2f);
            }
        }
        else
            okToJump_ = true;
    }

    if ( !onGround_ )
    {
        animCtrl->PlayExclusive("models/mutant/mutant_jump1.ani", 0, false, 0.2f);
    }
    else
    {
        // Play walk animation if moving on ground, otherwise fade it out
        if (softGrounded && !moveDir.Equals(Vector3::ZERO))
            animCtrl->PlayExclusive("models/mutant/mutant_run.ani", 0, true, 0.2f);
        else
            animCtrl->PlayExclusive("models/mutant/mutant_idle0.ani", 0, true, 0.2f);

        // Set walk animation speed proportional to velocity
        animCtrl->SetSpeed("models/mutant/mutant_run.ani", planeVelocity.Length() * 0.3f);
    }

    // Reset grounded flag for next frame
    onGround_ = false;
}

void Character::HandleNodeCollision(StringHash eventType, VariantMap& eventData)
{
    // Check collision contacts and see if character is standing on ground (look for a contact that has near vertical normal)
    using namespace NodeCollision;

    MemoryBuffer contacts(eventData[P_CONTACTS].GetBuffer());

    while (!contacts.IsEof())
    {
        Vector3 contactPosition = contacts.ReadVector3();
        Vector3 contactNormal = contacts.ReadVector3();
        /*float contactDistance = */contacts.ReadFloat();
        /*float contactImpulse = */contacts.ReadFloat();

        // If contact is below node center and pointing up, assume it's a ground contact
        if (contactPosition.y < (node_->GetPosition().y + 1.0f))
        {
            float level = contactNormal.y;
            if (level > 0.75)
                onGround_ = true;
        }
    }
}
