// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/context.h>
#include <dviglo/graphics/debug_renderer.h>
#include <dviglo/graphics/decal_set.h>
#include <dviglo/graphics/material.h>
#include <dviglo/graphics/model.h>
#include <dviglo/graphics/particle_effect.h>
#include <dviglo/graphics/particle_emitter.h>
#include <dviglo/graphics/static_model.h>
#include <dviglo/io/log.h>
#include <dviglo/physics/collision_shape.h>
#include <dviglo/physics/constraint.h>
#include <dviglo/physics/physics_events.h>
#include <dviglo/physics/physics_world.h>
#include <dviglo/physics/raycast_vehicle.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>

#include "vehicle.h"

using namespace dviglo;

const float CHASSIS_WIDTH = 2.6f;

void Vehicle::register_object()
{
    DV_CONTEXT.RegisterFactory<Vehicle>();
    DV_ATTRIBUTE("Steering", steering_, 0.0f, AM_DEFAULT);
    DV_ATTRIBUTE("Controls Yaw", controls_.yaw_, 0.0f, AM_DEFAULT);
    DV_ATTRIBUTE("Controls Pitch", controls_.pitch_, 0.0f, AM_DEFAULT);
}

Vehicle::Vehicle()
    : steering_(0.0f)
{
    SetUpdateEventMask(LogicComponentEvents::FixedUpdate | LogicComponentEvents::PostUpdate);
    engineForce_ = 0.0f;
    brakingForce_ = 50.0f;
    vehicleSteering_ = 0.0f;
    maxEngineForce_ = 2500.0f;
    wheelRadius_ = 0.5f;
    suspensionRestLength_ = 0.6f;
    wheelWidth_ = 0.4f;
    suspensionStiffness_ = 14.0f;
    suspensionDamping_ = 2.0f;
    suspensionCompression_ = 4.0f;
    wheelFriction_ = 1000.0f;
    rollInfluence_ = 0.12f;
    emittersCreated = false;
}

Vehicle::~Vehicle() = default;

void Vehicle::Init()
{
    auto* vehicle = node_->create_component<RaycastVehicle>();
    vehicle->Init();
    auto* hullBody = node_->GetComponent<RigidBody>();
    hullBody->SetMass(800.0f);
    hullBody->SetLinearDamping(0.2f); // Some air resistance
    hullBody->SetAngularDamping(0.5f);
    hullBody->SetCollisionLayer(1);
    // This function is called only from the main program when initially creating the vehicle, not on scene load
    ResourceCache& cache = DV_RES_CACHE;
    auto* hullObject = node_->create_component<StaticModel>();
    // Setting-up collision shape
    auto* hullColShape = node_->create_component<CollisionShape>();
    Vector3 v3BoxExtents = Vector3::ONE;
    hullColShape->SetBox(v3BoxExtents);
    node_->SetScale(Vector3(2.3f, 1.0f, 4.0f));
    hullObject->SetModel(cache.GetResource<Model>("Models/Box.mdl"));
    hullObject->SetMaterial(cache.GetResource<Material>("materials/Stone.xml"));
    hullObject->SetCastShadows(true);
    float connectionHeight = -0.4f;
    bool isFrontWheel = true;
    Vector3 wheelDirection(0, -1, 0);
    Vector3 wheelAxle(-1, 0, 0);
    // We use not scaled coordinates here as everything will be scaled.
    // Wheels are on bottom at edges of the chassis
    // Note we don't set wheel nodes as children of hull (while we could) to avoid scaling to affect them.
    float wheelX = CHASSIS_WIDTH / 2.0f - wheelWidth_;
    // Front left
    connectionPoints_[0] = Vector3(-wheelX, connectionHeight, 2.5f - GetWheelRadius() * 2.0f);
    // Front right
    connectionPoints_[1] = Vector3(wheelX, connectionHeight, 2.5f - GetWheelRadius() * 2.0f);
    // Back left
    connectionPoints_[2] = Vector3(-wheelX, connectionHeight, -2.5f + GetWheelRadius() * 2.0f);
    // Back right
    connectionPoints_[3] = Vector3(wheelX, connectionHeight, -2.5f + GetWheelRadius() * 2.0f);
    const Color LtBrown(0.972f, 0.780f, 0.412f);
    for (int id = 0; id < sizeof(connectionPoints_) / sizeof(connectionPoints_[0]); id++)
    {
        Node* wheelNode = GetScene()->create_child();
        Vector3 connectionPoint = connectionPoints_[id];
        // Front wheels are at front (z > 0)
        // back wheels are at z < 0
        // Setting rotation according to wheel position
        bool isFrontWheel = connectionPoints_[id].z > 0.0f;
        wheelNode->SetRotation(connectionPoint.x >= 0.0 ? Quaternion(0.0f, 0.0f, -90.0f) : Quaternion(0.0f, 0.0f, 90.0f));
        wheelNode->SetWorldPosition(node_->GetWorldPosition() + node_->GetWorldRotation() * connectionPoints_[id]);
        vehicle->AddWheel(wheelNode, wheelDirection, wheelAxle, suspensionRestLength_, wheelRadius_, isFrontWheel);
        vehicle->SetWheelSuspensionStiffness(id, suspensionStiffness_);
        vehicle->SetWheelDampingRelaxation(id, suspensionDamping_);
        vehicle->SetWheelDampingCompression(id, suspensionCompression_);
        vehicle->SetWheelFrictionSlip(id, wheelFriction_);
        vehicle->SetWheelRollInfluence(id, rollInfluence_);
        wheelNode->SetScale(Vector3(1.0f, 0.65f, 1.0f));
        auto* pWheel = wheelNode->create_component<StaticModel>();
        pWheel->SetModel(cache.GetResource<Model>("Models/Cylinder.mdl"));
        pWheel->SetMaterial(cache.GetResource<Material>("materials/Stone.xml"));
        pWheel->SetCastShadows(true);
        CreateEmitter(connectionPoints_[id]);
    }
    emittersCreated = true;
    vehicle->ResetWheels();
}

void Vehicle::CreateEmitter(Vector3 place)
{
    Node* emitter = GetScene()->create_child();
    emitter->SetWorldPosition(node_->GetWorldPosition() + node_->GetWorldRotation() * place + Vector3(0, -wheelRadius_, 0));
    auto* particleEmitter = emitter->create_component<ParticleEmitter>();
    particleEmitter->SetEffect(DV_RES_CACHE.GetResource<ParticleEffect>("particle/dust.xml"));
    particleEmitter->SetEmitting(false);
    particleEmitterNodeList_.Push(emitter);
    emitter->SetTemporary(true);
}

/// Applying attributes
void Vehicle::apply_attributes()
{
    auto* vehicle = node_->GetOrCreateComponent<RaycastVehicle>();
    if (emittersCreated)
        return;
    for (const auto& connectionPoint : connectionPoints_)
    {
        CreateEmitter(connectionPoint);
    }
    emittersCreated = true;
}

void Vehicle::FixedUpdate(float timeStep)
{
    float newSteering = 0.0f;
    float accelerator = 0.0f;
    bool brake = false;
    auto* vehicle = node_->GetComponent<RaycastVehicle>();
    // Read controls
    if (controls_.buttons_ & CTRL_LEFT)
    {
        newSteering = -1.0f;
    }
    if (controls_.buttons_ & CTRL_RIGHT)
    {
        newSteering = 1.0f;
    }
    if (controls_.buttons_ & CTRL_FORWARD)
    {
        accelerator = 1.0f;
    }
    if (controls_.buttons_ & CTRL_BACK)
    {
        accelerator = -0.5f;
    }
    if (controls_.buttons_ & CTRL_BRAKE)
    {
        brake = true;
    }
    // When steering, wake up the wheel rigidbodies so that their orientation is updated
    if (newSteering != 0.0f)
    {
        SetSteering(GetSteering() * 0.95f + newSteering * 0.05f);
    }
    else
    {
        SetSteering(GetSteering() * 0.8f + newSteering * 0.2f);
    }
    // Set front wheel angles
    vehicleSteering_ = steering_;
    int wheelIndex = 0;
    vehicle->SetSteeringValue(wheelIndex, vehicleSteering_);
    wheelIndex = 1;
    vehicle->SetSteeringValue(wheelIndex, vehicleSteering_);
    // apply forces
    engineForce_ = maxEngineForce_ * accelerator;
    // 2x wheel drive
    vehicle->SetEngineForce(2, engineForce_);
    vehicle->SetEngineForce(3, engineForce_);
    for (int i = 0; i < vehicle->GetNumWheels(); i++)
    {
        if (brake)
        {
            vehicle->SetBrake(i, brakingForce_);
        }
        else
        {
            vehicle->SetBrake(i, 0.0f);
        }
    }
}

void Vehicle::PostUpdate(float timeStep)
{
    auto* vehicle = node_->GetComponent<RaycastVehicle>();
    auto* vehicleBody = node_->GetComponent<RigidBody>();
    Vector3 velocity = vehicleBody->GetLinearVelocity();
    Vector3 accel = (velocity - prevVelocity_) / timeStep;
    float planeAccel = Vector3(accel.x, 0.0f, accel.z).Length();
    for (int i = 0; i < vehicle->GetNumWheels(); i++)
    {
        Node* emitter = particleEmitterNodeList_[i];
        auto* particleEmitter = emitter->GetComponent<ParticleEmitter>();
        if (vehicle->WheelIsGrounded(i) && (vehicle->GetWheelSkidInfoCumulative(i) < 0.9f || vehicle->GetBrake(i) > 2.0f ||
            planeAccel > 15.0f))
        {
            particleEmitterNodeList_[i]->SetWorldPosition(vehicle->GetContactPosition(i));
            if (!particleEmitter->IsEmitting())
            {
                particleEmitter->SetEmitting(true);
            }
            DV_LOGDEBUG("GetWheelSkidInfoCumulative() = " +
                            String(vehicle->GetWheelSkidInfoCumulative(i)) + " " +
                            String(vehicle->GetMaxSideSlipSpeed()));
            /* TODO: Add skid marks here */
        }
        else if (particleEmitter->IsEmitting())
        {
            particleEmitter->SetEmitting(false);
        }
    }
    prevVelocity_ = velocity;
}
