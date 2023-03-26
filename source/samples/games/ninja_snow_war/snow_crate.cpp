// Copyright (c) 2022-2023 the Dviglo project
// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#include "snow_crate.h"
#include "utilities/spawn.h"

namespace dviglo
{

static constexpr i32 SNOWCRATE_HEALTH = 5;
static constexpr i32 SNOWCRATE_POINTS = 250;

void SnowCrate::register_object()
{
    DV_CONTEXT.RegisterFactory<SnowCrate>();
}

SnowCrate::SnowCrate()
{
    health = maxHealth = SNOWCRATE_HEALTH;
}

void SnowCrate::Start()
{
    subscribe_to_event(node_, E_NODECOLLISION, DV_HANDLER(SnowCrate, HandleNodeCollision));
}

void SnowCrate::FixedUpdate(float timeStep)
{
    if (health <= 0)
    {
        SpawnParticleEffect(node_->GetScene(), node_->GetPosition(), "Particle/SnowExplosionBig.xml", 2);
        SpawnObject(node_->GetScene(), node_->GetPosition(), Quaternion(), "potion");

        VariantMap eventData;
        eventData["Points"] = SNOWCRATE_POINTS;
        eventData["Receiver"] = lastDamageCreatorID;
        eventData["DamageSide"] = lastDamageSide;
        SendEvent("Points", eventData);

        node_->Remove();
    }
}

} // namespace dviglo
