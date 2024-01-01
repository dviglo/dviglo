// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#include <dviglo/core/context.h>
#include <dviglo/graphics/material.h>
#include <dviglo/graphics/model.h>
#include <dviglo/graphics/static_model.h>
#include <dviglo/physics/collision_shape.h>
#include <dviglo/physics/constraint.h>
#include <dviglo/physics/physics_events.h>
#include <dviglo/physics/physics_world.h>
#include <dviglo/physics/rigid_body.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>

#include "vehicle.h"

Vehicle::Vehicle()
{
    // Only the physics update event is needed: unsubscribe from the rest for optimization
    SetUpdateEventMask(LogicComponentEvents::FixedUpdate);
}

void Vehicle::register_object()
{
    DV_CONTEXT->RegisterFactory<Vehicle>();

    DV_ATTRIBUTE("Controls Yaw", controls_.yaw_, 0.0f, AM_DEFAULT);
    DV_ATTRIBUTE("Controls Pitch", controls_.pitch_, 0.0f, AM_DEFAULT);
    DV_ATTRIBUTE("Steering", steering_, 0.0f, AM_DEFAULT);
    // Register wheel node IDs as attributes so that the wheel nodes can be reaquired on deserialization. They need to be tagged
    // as node ID's so that the deserialization code knows to rewrite the IDs in case they are different on load than on save
    DV_ATTRIBUTE("Front Left Node", frontLeftID_, 0, AM_DEFAULT | AM_NODEID);
    DV_ATTRIBUTE("Front Right Node", frontRightID_, 0, AM_DEFAULT | AM_NODEID);
    DV_ATTRIBUTE("Rear Left Node", rearLeftID_, 0, AM_DEFAULT | AM_NODEID);
    DV_ATTRIBUTE("Rear Right Node", rearRightID_, 0, AM_DEFAULT | AM_NODEID);
}

void Vehicle::apply_attributes()
{
    // This function is called on each Serializable after the whole scene has been loaded. Reacquire wheel nodes from ID's
    // as well as all required physics components
    Scene* scene = GetScene();

    frontLeft_ = scene->GetNode(frontLeftID_);
    frontRight_ = scene->GetNode(frontRightID_);
    rearLeft_ = scene->GetNode(rearLeftID_);
    rearRight_ = scene->GetNode(rearRightID_);
    hullBody_ = node_->GetComponent<RigidBody>();

    GetWheelComponents();
}

void Vehicle::FixedUpdate(float timeStep)
{
    float newSteering = 0.0f;
    float accelerator = 0.0f;

    // Read controls
    if (controls_.buttons_ & CTRL_LEFT)
        newSteering = -1.0f;
    if (controls_.buttons_ & CTRL_RIGHT)
        newSteering = 1.0f;
    if (controls_.buttons_ & CTRL_FORWARD)
        accelerator = 1.0f;
    if (controls_.buttons_ & CTRL_BACK)
        accelerator = -0.5f;

    // When steering, wake up the wheel rigidbodies so that their orientation is updated
    if (newSteering != 0.0f)
    {
        frontLeftBody_->Activate();
        frontRightBody_->Activate();
        steering_ = steering_ * 0.95f + newSteering * 0.05f;
    }
    else
        steering_ = steering_ * 0.8f + newSteering * 0.2f;

    // Set front wheel angles
    Quaternion steeringRot(0, steering_ * MAX_WHEEL_ANGLE, 0);
    frontLeftAxis_->SetOtherAxis(steeringRot * Vector3::LEFT);
    frontRightAxis_->SetOtherAxis(steeringRot * Vector3::RIGHT);

    Quaternion hullRot = hullBody_->GetRotation();
    if (accelerator != 0.0f)
    {
        // Torques are applied in world space, so need to take the vehicle & wheel rotation into account
        Vector3 torqueVec = Vector3(ENGINE_POWER * accelerator, 0.0f, 0.0f);

        frontLeftBody_->ApplyTorque(hullRot * steeringRot * torqueVec);
        frontRightBody_->ApplyTorque(hullRot * steeringRot * torqueVec);
        rearLeftBody_->ApplyTorque(hullRot * torqueVec);
        rearRightBody_->ApplyTorque(hullRot * torqueVec);
    }

    // Apply downforce proportional to velocity
    Vector3 localVelocity = hullRot.Inverse() * hullBody_->GetLinearVelocity();
    hullBody_->ApplyForce(hullRot * Vector3::DOWN * Abs(localVelocity.z) * DOWN_FORCE);
}

void Vehicle::Init()
{
    // This function is called only from the main program when initially creating the vehicle, not on scene load
    auto* hullObject = node_->create_component<StaticModel>();
    hullBody_ = node_->create_component<RigidBody>();
    auto* hullShape = node_->create_component<CollisionShape>();

    node_->SetScale(Vector3(1.5f, 1.0f, 3.0f));
    hullObject->SetModel(DV_RES_CACHE->GetResource<Model>("models/box.mdl"));
    hullObject->SetMaterial(DV_RES_CACHE->GetResource<Material>("materials/stone.xml"));
    hullObject->SetCastShadows(true);
    hullShape->SetBox(Vector3::ONE);
    hullBody_->SetMass(4.0f);
    hullBody_->SetLinearDamping(0.2f); // Some air resistance
    hullBody_->SetAngularDamping(0.5f);
    hullBody_->SetCollisionLayer(1);

    InitWheel("FrontLeft", Vector3(-0.6f, -0.4f, 0.3f), frontLeft_, frontLeftID_);
    InitWheel("FrontRight", Vector3(0.6f, -0.4f, 0.3f), frontRight_, frontRightID_);
    InitWheel("RearLeft", Vector3(-0.6f, -0.4f, -0.3f), rearLeft_, rearLeftID_);
    InitWheel("RearRight", Vector3(0.6f, -0.4f, -0.3f), rearRight_, rearRightID_);

    GetWheelComponents();
}

void Vehicle::InitWheel(const String& name, const Vector3& offset, WeakPtr<Node>& wheelNode, unsigned& wheelNodeID)
{
    // Note: do not parent the wheel to the hull scene node. Instead create it on the root level and let the physics
    // constraint keep it together
    wheelNode = GetScene()->create_child(name);
    wheelNode->SetPosition(node_->LocalToWorld(offset));
    wheelNode->SetRotation(node_->GetRotation() * (offset.x >= 0.0 ? Quaternion(0.0f, 0.0f, -90.0f) :
        Quaternion(0.0f, 0.0f, 90.0f)));
    wheelNode->SetScale(Vector3(0.8f, 0.5f, 0.8f));
    // Remember the ID for serialization
    wheelNodeID = wheelNode->GetID();

    auto* wheelObject = wheelNode->create_component<StaticModel>();
    auto* wheelBody = wheelNode->create_component<RigidBody>();
    auto* wheelShape = wheelNode->create_component<CollisionShape>();
    auto* wheelConstraint = wheelNode->create_component<Constraint>();

    wheelObject->SetModel(DV_RES_CACHE->GetResource<Model>("models/cylinder.mdl"));
    wheelObject->SetMaterial(DV_RES_CACHE->GetResource<Material>("materials/stone.xml"));
    wheelObject->SetCastShadows(true);
    wheelShape->SetSphere(1.0f);
    wheelBody->SetFriction(1.0f);
    wheelBody->SetMass(1.0f);
    wheelBody->SetLinearDamping(0.2f); // Some air resistance
    wheelBody->SetAngularDamping(0.75f); // Could also use rolling friction
    wheelBody->SetCollisionLayer(1);
    wheelConstraint->SetConstraintType(CONSTRAINT_HINGE);
    wheelConstraint->SetOtherBody(GetComponent<RigidBody>()); // Connect to the hull body
    wheelConstraint->SetWorldPosition(wheelNode->GetPosition()); // Set constraint's both ends at wheel's location
    wheelConstraint->SetAxis(Vector3::UP); // Wheel rotates around its local Y-axis
    wheelConstraint->SetOtherAxis(offset.x >= 0.0 ? Vector3::RIGHT : Vector3::LEFT); // Wheel's hull axis points either left or right
    wheelConstraint->SetLowLimit(Vector2(-180.0f, 0.0f)); // Let the wheel rotate freely around the axis
    wheelConstraint->SetHighLimit(Vector2(180.0f, 0.0f));
    wheelConstraint->SetDisableCollision(true); // Let the wheel intersect the vehicle hull
}

void Vehicle::GetWheelComponents()
{
    frontLeftAxis_ = frontLeft_->GetComponent<Constraint>();
    frontRightAxis_ = frontRight_->GetComponent<Constraint>();
    frontLeftBody_ = frontLeft_->GetComponent<RigidBody>();
    frontRightBody_ = frontRight_->GetComponent<RigidBody>();
    rearLeftBody_ = rearLeft_->GetComponent<RigidBody>();
    rearRightBody_ = rearRight_->GetComponent<RigidBody>();
}
