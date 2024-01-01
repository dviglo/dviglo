// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "../core/object.h"

// For prestep / poststep events, which are the same for 2D and 3D physics. The events themselves don't depend
// on whether 3D physics support or Bullet has been compiled in.
#include "../physics/physics_events.h"

namespace dviglo
{

/// Physics update contact. Global event sent by PhysicsWorld2D.
DV_EVENT(E_PHYSICSUPDATECONTACT2D, PhysicsUpdateContact2D)
{
    DV_PARAM(P_WORLD, World);                  // PhysicsWorld2D pointer
    DV_PARAM(P_BODYA, BodyA);                  // RigidBody2D pointer
    DV_PARAM(P_BODYB, BodyB);                  // RigidBody2D pointer
    DV_PARAM(P_NODEA, NodeA);                  // Node pointer
    DV_PARAM(P_NODEB, NodeB);                  // Node pointer
    DV_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector2), normal (Vector2), negative overlap distance (float). Normal is the same for all points.
    DV_PARAM(P_SHAPEA, ShapeA);                // CollisionShape2D pointer
    DV_PARAM(P_SHAPEB, ShapeB);                // CollisionShape2D pointer
    DV_PARAM(P_ENABLED, Enabled);              // bool [in/out]
}

/// Physics begin contact. Global event sent by PhysicsWorld2D.
DV_EVENT(E_PHYSICSBEGINCONTACT2D, PhysicsBeginContact2D)
{
    DV_PARAM(P_WORLD, World);                  // PhysicsWorld2D pointer
    DV_PARAM(P_BODYA, BodyA);                  // RigidBody2D pointer
    DV_PARAM(P_BODYB, BodyB);                  // RigidBody2D pointer
    DV_PARAM(P_NODEA, NodeA);                  // Node pointer
    DV_PARAM(P_NODEB, NodeB);                  // Node pointer
    DV_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector2), normal (Vector2), negative overlap distance (float). Normal is the same for all points.
    DV_PARAM(P_SHAPEA, ShapeA);                // CollisionShape2D pointer
    DV_PARAM(P_SHAPEB, ShapeB);                // CollisionShape2D pointer
}

/// Physics end contact. Global event sent by PhysicsWorld2D.
DV_EVENT(E_PHYSICSENDCONTACT2D, PhysicsEndContact2D)
{
    DV_PARAM(P_WORLD, World);                  // PhysicsWorld2D pointer
    DV_PARAM(P_BODYA, BodyA);                  // RigidBody2D pointer
    DV_PARAM(P_BODYB, BodyB);                  // RigidBody2D pointer
    DV_PARAM(P_NODEA, NodeA);                  // Node pointer
    DV_PARAM(P_NODEB, NodeB);                  // Node pointer
    DV_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector2), normal (Vector2), negative overlap distance (float). Normal is the same for all points.
    DV_PARAM(P_SHAPEA, ShapeA);                // CollisionShape2D pointer
    DV_PARAM(P_SHAPEB, ShapeB);                // CollisionShape2D pointer
}

/// Node update contact. Sent by scene nodes participating in a collision.
DV_EVENT(E_NODEUPDATECONTACT2D, NodeUpdateContact2D)
{
    DV_PARAM(P_BODY, Body);                    // RigidBody2D pointer
    DV_PARAM(P_OTHERNODE, OtherNode);          // Node pointer
    DV_PARAM(P_OTHERBODY, OtherBody);          // RigidBody2D pointer
    DV_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector2), normal (Vector2), negative overlap distance (float). Normal is the same for all points.
    DV_PARAM(P_SHAPE, Shape);                  // CollisionShape2D pointer
    DV_PARAM(P_OTHERSHAPE, OtherShape);        // CollisionShape2D pointer
    DV_PARAM(P_ENABLED, Enabled);              // bool [in/out]
}

/// Node begin contact. Sent by scene nodes participating in a collision.
DV_EVENT(E_NODEBEGINCONTACT2D, NodeBeginContact2D)
{
    DV_PARAM(P_BODY, Body);                    // RigidBody2D pointer
    DV_PARAM(P_OTHERNODE, OtherNode);          // Node pointer
    DV_PARAM(P_OTHERBODY, OtherBody);          // RigidBody2D pointer
    DV_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector2), normal (Vector2), negative overlap distance (float). Normal is the same for all points.
    DV_PARAM(P_SHAPE, Shape);                  // CollisionShape2D pointer
    DV_PARAM(P_OTHERSHAPE, OtherShape);        // CollisionShape2D pointer
}

/// Node end contact. Sent by scene nodes participating in a collision.
DV_EVENT(E_NODEENDCONTACT2D, NodeEndContact2D)
{
    DV_PARAM(P_BODY, Body);                    // RigidBody2D pointer
    DV_PARAM(P_OTHERNODE, OtherNode);          // Node pointer
    DV_PARAM(P_OTHERBODY, OtherBody);          // RigidBody2D pointer
    DV_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector2), normal (Vector2), negative overlap distance (float). Normal is the same for all points.
    DV_PARAM(P_SHAPE, Shape);                  // CollisionShape2D pointer
    DV_PARAM(P_OTHERSHAPE, OtherShape);        // CollisionShape2D pointer
}

}
