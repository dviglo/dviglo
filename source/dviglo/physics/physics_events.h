// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

namespace dviglo
{

/// Physics world is about to be stepped.
DV_EVENT(E_PHYSICSPRESTEP, PhysicsPreStep)
{
    DV_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    DV_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Physics world has been stepped.
DV_EVENT(E_PHYSICSPOSTSTEP, PhysicsPostStep)
{
    DV_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    DV_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Physics collision started. Global event sent by the PhysicsWorld.
DV_EVENT(E_PHYSICSCOLLISIONSTART, PhysicsCollisionStart)
{
    DV_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    DV_PARAM(P_NODEA, NodeA);                  // Node pointer
    DV_PARAM(P_NODEB, NodeB);                  // Node pointer
    DV_PARAM(P_BODYA, BodyA);                  // RigidBody pointer
    DV_PARAM(P_BODYB, BodyB);                  // RigidBody pointer
    DV_PARAM(P_TRIGGER, Trigger);              // bool
    DV_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector3), normal (Vector3), distance (float), impulse (float) for each contact
}

/// Physics collision ongoing. Global event sent by the PhysicsWorld.
DV_EVENT(E_PHYSICSCOLLISION, PhysicsCollision)
{
    DV_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    DV_PARAM(P_NODEA, NodeA);                  // Node pointer
    DV_PARAM(P_NODEB, NodeB);                  // Node pointer
    DV_PARAM(P_BODYA, BodyA);                  // RigidBody pointer
    DV_PARAM(P_BODYB, BodyB);                  // RigidBody pointer
    DV_PARAM(P_TRIGGER, Trigger);              // bool
    DV_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector3), normal (Vector3), distance (float), impulse (float) for each contact
}

/// Physics collision ended. Global event sent by the PhysicsWorld.
DV_EVENT(E_PHYSICSCOLLISIONEND, PhysicsCollisionEnd)
{
    DV_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    DV_PARAM(P_NODEA, NodeA);                  // Node pointer
    DV_PARAM(P_NODEB, NodeB);                  // Node pointer
    DV_PARAM(P_BODYA, BodyA);                  // RigidBody pointer
    DV_PARAM(P_BODYB, BodyB);                  // RigidBody pointer
    DV_PARAM(P_TRIGGER, Trigger);              // bool
}

/// Node's physics collision started. Sent by scene nodes participating in a collision.
DV_EVENT(E_NODECOLLISIONSTART, NodeCollisionStart)
{
    DV_PARAM(P_BODY, Body);                    // RigidBody pointer
    DV_PARAM(P_OTHERNODE, OtherNode);          // Node pointer
    DV_PARAM(P_OTHERBODY, OtherBody);          // RigidBody pointer
    DV_PARAM(P_TRIGGER, Trigger);              // bool
    DV_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector3), normal (Vector3), distance (float), impulse (float) for each contact
}

/// Node's physics collision ongoing. Sent by scene nodes participating in a collision.
DV_EVENT(E_NODECOLLISION, NodeCollision)
{
    DV_PARAM(P_BODY, Body);                    // RigidBody pointer
    DV_PARAM(P_OTHERNODE, OtherNode);          // Node pointer
    DV_PARAM(P_OTHERBODY, OtherBody);          // RigidBody pointer
    DV_PARAM(P_TRIGGER, Trigger);              // bool
    DV_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector3), normal (Vector3), distance (float), impulse (float) for each contact
}

/// Node's physics collision ended. Sent by scene nodes participating in a collision.
DV_EVENT(E_NODECOLLISIONEND, NodeCollisionEnd)
{
    DV_PARAM(P_BODY, Body);                    // RigidBody pointer
    DV_PARAM(P_OTHERNODE, OtherNode);          // Node pointer
    DV_PARAM(P_OTHERBODY, OtherBody);          // RigidBody pointer
    DV_PARAM(P_TRIGGER, Trigger);              // bool
}

}
