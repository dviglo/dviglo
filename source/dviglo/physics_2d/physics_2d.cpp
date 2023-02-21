// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "collision_box_2d.h"
#include "collision_chain_2d.h"
#include "collision_circle_2d.h"
#include "collision_edge_2d.h"
#include "collision_polygon_2d.h"
#include "constraint_2d.h"
#include "constraint_distance_2d.h"
#include "constraint_friction_2d.h"
#include "constraint_gear_2d.h"
#include "constraint_motor_2d.h"
#include "constraint_mouse_2d.h"
#include "constraint_prismatic_2d.h"
#include "constraint_pulley_2d.h"
#include "constraint_revolute_2d.h"
#include "constraint_weld_2d.h"
#include "constraint_wheel_2d.h"
#include "physics_2d.h"
#include "physics_world_2d.h"
#include "rigid_body_2d.h"

#include "../common/debug_new.h"

namespace dviglo
{

const char* PHYSICS2D_CATEGORY = "Physics2D";

void RegisterPhysics2DLibrary()
{
    PhysicsWorld2D::RegisterObject();
    RigidBody2D::RegisterObject();

    CollisionShape2D::RegisterObject();
    CollisionBox2D::RegisterObject();
    CollisionChain2D::RegisterObject();
    CollisionCircle2D::RegisterObject();
    CollisionEdge2D::RegisterObject();
    CollisionPolygon2D::RegisterObject();

    Constraint2D::RegisterObject();
    ConstraintDistance2D::RegisterObject();
    ConstraintFriction2D::RegisterObject();
    ConstraintGear2D::RegisterObject();
    ConstraintMotor2D::RegisterObject();
    ConstraintMouse2D::RegisterObject();
    ConstraintPrismatic2D::RegisterObject();
    ConstraintPulley2D::RegisterObject();
    ConstraintRevolute2D::RegisterObject();
    ConstraintWeld2D::RegisterObject();
    ConstraintWheel2D::RegisterObject();
}

}
