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

void RegisterPhysics2DLibrary(Context* context)
{
    PhysicsWorld2D::RegisterObject(context);
    RigidBody2D::RegisterObject(context);

    CollisionShape2D::RegisterObject(context);
    CollisionBox2D::RegisterObject(context);
    CollisionChain2D::RegisterObject(context);
    CollisionCircle2D::RegisterObject(context);
    CollisionEdge2D::RegisterObject(context);
    CollisionPolygon2D::RegisterObject(context);

    Constraint2D::RegisterObject(context);
    ConstraintDistance2D::RegisterObject(context);
    ConstraintFriction2D::RegisterObject(context);
    ConstraintGear2D::RegisterObject(context);
    ConstraintMotor2D::RegisterObject(context);
    ConstraintMouse2D::RegisterObject(context);
    ConstraintPrismatic2D::RegisterObject(context);
    ConstraintPulley2D::RegisterObject(context);
    ConstraintRevolute2D::RegisterObject(context);
    ConstraintWeld2D::RegisterObject(context);
    ConstraintWheel2D::RegisterObject(context);
}

}
