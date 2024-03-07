// Copyright (c) the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#include "snowball.h"
#include "utilities/spawn.h"

static constexpr float SNOWBALL_MIN_HIT_SPEED = 1.f;
static constexpr float SNOWBALL_DAMPING_FORCE = 20.f;
static constexpr float SNOWBALL_DURATION = 5.f;
static constexpr float SNOWBALL_GROUND_HIT_DURATION = 1.f;
static constexpr float SNOWBALL_OBJECT_HIT_DURATION = 0.f;
static constexpr i32 SNOWBALL_DAMAGE = 1;

void Snowball::register_object()
{
    DV_CONTEXT->RegisterFactory<Snowball>();
}

Snowball::Snowball()
{
    duration = SNOWBALL_DURATION;
    hitDamage = SNOWBALL_DAMAGE;
}

void Snowball::Start()
{
    subscribe_to_event(node_, E_NODECOLLISION, DV_HANDLER(Snowball, HandleNodeCollision));
}

void Snowball::FixedUpdate(float timeStep)
{
    // Apply damping when rolling on the ground, or near disappearing
    RigidBody* body = node_->GetComponent<RigidBody>();

    if (onGround || duration < SNOWBALL_GROUND_HIT_DURATION)
    {
        Vector3 vel = body->GetLinearVelocity();
        body->ApplyForce(Vector3(-SNOWBALL_DAMPING_FORCE * vel.x, 0.f, -SNOWBALL_DAMPING_FORCE * vel.z));
    }

    // Disappear when duration expired
    if (duration >= 0.f)
    {
        duration -= timeStep;
        if (duration <= 0.f)
        {
            SpawnParticleEffect(node_->GetScene(), node_->GetPosition(), "particle/snow_explosion.xml", 1);
            node_->Remove();
        }
    }
}

void Snowball::WorldCollision(VariantMap& eventData)
{
    GameObject::WorldCollision(eventData);

    // If hit the ground, disappear after a short while
    if (duration > SNOWBALL_GROUND_HIT_DURATION)
        duration = SNOWBALL_GROUND_HIT_DURATION;
}

void Snowball::ObjectCollision(GameObject& otherObject, VariantMap& eventData)
{
    if (hitDamage > 0)
    {
        RigidBody* body = node_->GetComponent<RigidBody>();

        if (body->GetLinearVelocity().Length() >= SNOWBALL_MIN_HIT_SPEED)
        {
            if (side != otherObject.side)
            {
                otherObject.Damage(*this, hitDamage);
                // Create a temporary node for the hit sound
                SpawnSound(node_->GetScene(), node_->GetPosition(), "sounds/player_fist_hit.wav", 0.2f);
            }

            hitDamage = 0;
        }
    }
    if (duration > SNOWBALL_OBJECT_HIT_DURATION)
        duration = SNOWBALL_OBJECT_HIT_DURATION;
}
