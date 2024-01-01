// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
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

void register_physics_2d_library()
{
    PhysicsWorld2D::register_object();
    RigidBody2D::register_object();

    CollisionShape2D::register_object();
    CollisionBox2D::register_object();
    CollisionChain2D::register_object();
    CollisionCircle2D::register_object();
    CollisionEdge2D::register_object();
    CollisionPolygon2D::register_object();

    Constraint2D::register_object();
    ConstraintDistance2D::register_object();
    ConstraintFriction2D::register_object();
    ConstraintGear2D::register_object();
    ConstraintMotor2D::register_object();
    ConstraintMouse2D::register_object();
    ConstraintPrismatic2D::register_object();
    ConstraintPulley2D::register_object();
    ConstraintRevolute2D::register_object();
    ConstraintWeld2D::register_object();
    ConstraintWheel2D::register_object();
}

}
