// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../precompiled.h"

#include "../core/context.h"
#include "../io/log.h"
#include "../physics_2d/collision_shape_2d.h"
#include "../physics_2d/constraint_2d.h"
#include "../physics_2d/physics_utils_2d.h"
#include "../physics_2d/physics_world_2d.h"
#include "../physics_2d/rigid_body_2d.h"
#include "../scene/scene.h"

#include "../debug_new.h"

namespace dviglo
{

extern const char* PHYSICS2D_CATEGORY;
static const BodyType2D DEFAULT_BODYTYPE = BT_STATIC;

static const char* bodyTypeNames[] =
{
    "Static",
    "Kinematic",
    "Dynamic",
    nullptr
};

RigidBody2D::RigidBody2D(Context* context) :
    Component(context),
    useFixtureMass_(true),
    body_(nullptr)
{
    // Make sure the massData members are zero-initialized.
    massData_.mass = 0.0f;
    massData_.I = 0.0f;
    massData_.center.SetZero();
}

RigidBody2D::~RigidBody2D()
{
    if (physicsWorld_)
    {
        ReleaseBody();

        physicsWorld_->RemoveRigidBody(this);
    }
}

void RigidBody2D::RegisterObject(Context* context)
{
    context->RegisterFactory<RigidBody2D>(PHYSICS2D_CATEGORY);

    DV_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, true, AM_DEFAULT);
    DV_ENUM_ACCESSOR_ATTRIBUTE("Body Type", GetBodyType, SetBodyType, bodyTypeNames, DEFAULT_BODYTYPE, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Mass", GetMass, SetMass, 0.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Inertia", GetInertia, SetInertia, 0.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Mass Center", GetMassCenter, SetMassCenter, Vector2::ZERO, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Use Fixture Mass", GetUseFixtureMass, SetUseFixtureMass, true, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Linear Damping", GetLinearDamping, SetLinearDamping, 0.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Angular Damping", GetAngularDamping, SetAngularDamping, 0.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Allow Sleep", IsAllowSleep, SetAllowSleep, true, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Fixed Rotation", IsFixedRotation, SetFixedRotation, false, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Bullet", IsBullet, SetBullet, false, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Gravity Scale", GetGravityScale, SetGravityScale, 1.0f, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Awake", IsAwake, SetAwake, true, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Linear Velocity", GetLinearVelocity, SetLinearVelocity, Vector2::ZERO, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Angular Velocity", GetAngularVelocity, SetAngularVelocity, 0.0f, AM_DEFAULT);
}


void RigidBody2D::OnSetEnabled()
{
    bool enabled = IsEnabledEffective();

    bodyDef_.enabled = enabled;

    if (body_)
        body_->SetEnabled(enabled);

    MarkNetworkUpdate();
}

void RigidBody2D::SetBodyType(BodyType2D type)
{
    auto bodyType = (b2BodyType)type;
    if (body_)
    {
        body_->SetType(bodyType);
        // Mass data was reset to keep it legal (e.g. static body should have mass 0).
        // If not using fixture mass, reassign our mass data now
        if (!useFixtureMass_)
            body_->SetMassData(&massData_);
    }
    else
    {
        if (bodyDef_.type == bodyType)
            return;

        bodyDef_.type = bodyType;
    }

    MarkNetworkUpdate();
}

void RigidBody2D::SetMass(float mass)
{
    mass = Max(mass, 0.0f);
    if (massData_.mass == mass)
        return;

    massData_.mass = mass;

    if (!useFixtureMass_ && body_)
        body_->SetMassData(&massData_);

    MarkNetworkUpdate();
}

void RigidBody2D::SetInertia(float inertia)
{
    inertia = Max(inertia, 0.0f);
    if (massData_.I == inertia)
        return;

    massData_.I = inertia;

    if (!useFixtureMass_ && body_)
        body_->SetMassData(&massData_);

    MarkNetworkUpdate();
}

void RigidBody2D::SetMassCenter(const Vector2& center)
{
    b2Vec2 b2Center = ToB2Vec2(center);
    if (massData_.center == b2Center)
        return;

    massData_.center = b2Center;

    if (!useFixtureMass_ && body_)
        body_->SetMassData(&massData_);

    MarkNetworkUpdate();
}

void RigidBody2D::SetUseFixtureMass(bool useFixtureMass)
{
    if (useFixtureMass_ == useFixtureMass)
        return;

    useFixtureMass_ = useFixtureMass;

    if (body_)
    {
        if (useFixtureMass_)
            body_->ResetMassData();
        else
            body_->SetMassData(&massData_);
    }

    MarkNetworkUpdate();
}

void RigidBody2D::SetLinearDamping(float linearDamping)
{
    if (body_)
        body_->SetLinearDamping(linearDamping);
    else
    {
        if (bodyDef_.linearDamping == linearDamping)
            return;

        bodyDef_.linearDamping = linearDamping;
    }

    MarkNetworkUpdate();
}

void RigidBody2D::SetAngularDamping(float angularDamping)
{
    if (body_)
        body_->SetAngularDamping(angularDamping);
    else
    {
        if (bodyDef_.angularDamping == angularDamping)
            return;

        bodyDef_.angularDamping = angularDamping;
    }

    MarkNetworkUpdate();
}

void RigidBody2D::SetAllowSleep(bool allowSleep)
{
    if (body_)
        body_->SetSleepingAllowed(allowSleep);
    else
    {
        if (bodyDef_.allowSleep == allowSleep)
            return;

        bodyDef_.allowSleep = allowSleep;
    }

    MarkNetworkUpdate();
}

void RigidBody2D::SetFixedRotation(bool fixedRotation)
{
    if (body_)
    {
        body_->SetFixedRotation(fixedRotation);
        // Mass data was reset to keep it legal (e.g. non-rotating body should have inertia 0).
        // If not using fixture mass, reassign our mass data now
        if (!useFixtureMass_)
            body_->SetMassData(&massData_);
    }
    else
    {
        if (bodyDef_.fixedRotation == fixedRotation)
            return;

        bodyDef_.fixedRotation = fixedRotation;
    }

    MarkNetworkUpdate();
}

void RigidBody2D::SetBullet(bool bullet)
{
    if (body_)
        body_->SetBullet(bullet);
    else
    {
        if (bodyDef_.bullet == bullet)
            return;

        bodyDef_.bullet = bullet;
    }

    MarkNetworkUpdate();
}

void RigidBody2D::SetGravityScale(float gravityScale)
{
    if (body_)
        body_->SetGravityScale(gravityScale);
    else
    {
        if (bodyDef_.gravityScale == gravityScale)
            return;

        bodyDef_.gravityScale = gravityScale;
    }

    MarkNetworkUpdate();
}

void RigidBody2D::SetAwake(bool awake)
{
    if (body_)
        body_->SetAwake(awake);
    else
    {
        if (bodyDef_.awake == awake)
            return;

        bodyDef_.awake = awake;
    }

    MarkNetworkUpdate();
}

void RigidBody2D::SetLinearVelocity(const Vector2& linearVelocity)
{
    b2Vec2 b2linearVelocity = ToB2Vec2(linearVelocity);
    if (body_)
        body_->SetLinearVelocity(b2linearVelocity);
    else
    {
        if (bodyDef_.linearVelocity == b2linearVelocity)
            return;

        bodyDef_.linearVelocity = b2linearVelocity;
    }

    MarkNetworkUpdate();
}

void RigidBody2D::SetAngularVelocity(float angularVelocity)
{
    if (body_)
        body_->SetAngularVelocity(angularVelocity);
    else
    {
        if (bodyDef_.angularVelocity == angularVelocity)
            return;

        bodyDef_.angularVelocity = angularVelocity;
    }

    MarkNetworkUpdate();
}

void RigidBody2D::ApplyForce(const Vector2& force, const Vector2& point, bool wake)
{
    if (body_ && force != Vector2::ZERO)
        body_->ApplyForce(ToB2Vec2(force), ToB2Vec2(point), wake);
}

void RigidBody2D::ApplyForceToCenter(const Vector2& force, bool wake)
{
    if (body_ && force != Vector2::ZERO)
        body_->ApplyForceToCenter(ToB2Vec2(force), wake);
}

void RigidBody2D::ApplyTorque(float torque, bool wake)
{
    if (body_ && torque != 0)
        body_->ApplyTorque(torque, wake);
}

void RigidBody2D::ApplyLinearImpulse(const Vector2& impulse, const Vector2& point, bool wake)
{
    if (body_ && impulse != Vector2::ZERO)
        body_->ApplyLinearImpulse(ToB2Vec2(impulse), ToB2Vec2(point), wake);
}

void RigidBody2D::ApplyLinearImpulseToCenter(const Vector2& impulse, bool wake)
{
    if (body_ && impulse != Vector2::ZERO)
        body_->ApplyLinearImpulseToCenter(ToB2Vec2(impulse), wake);
}

void RigidBody2D::ApplyAngularImpulse(float impulse, bool wake)
{
    if (body_)
        body_->ApplyAngularImpulse(impulse, wake);
}

void RigidBody2D::CreateBody()
{
    if (body_)
        return;

    if (!physicsWorld_ || !physicsWorld_->GetWorld())
        return;

    bodyDef_.position = ToB2Vec2(node_->GetWorldPosition());
    bodyDef_.angle = node_->GetWorldRotation().RollAngle() * M_DEGTORAD;

    body_ = physicsWorld_->GetWorld()->CreateBody(&bodyDef_);
    body_->GetUserData().pointer = (uintptr_t)this;

    for (const WeakPtr<CollisionShape2D>& collisionShape : collisionShapes_)
    {
        if (collisionShape)
            collisionShape->CreateFixture();
    }

    if (!useFixtureMass_)
        body_->SetMassData(&massData_);

    for (const WeakPtr<Constraint2D>& constraint : constraints_)
    {
        if (constraint)
            constraint->CreateJoint();
    }
}

void RigidBody2D::ReleaseBody()
{
    if (!body_)
        return;

    if (!physicsWorld_ || !physicsWorld_->GetWorld())
        return;

    // Make a copy for iteration
    Vector<WeakPtr<Constraint2D>> constraints = constraints_;

    for (const WeakPtr<Constraint2D>& constraint : constraints)
    {
        if (constraint)
            constraint->ReleaseJoint();
    }

    for (const WeakPtr<CollisionShape2D>& collisionShape : collisionShapes_)
    {
        if (collisionShape)
            collisionShape->ReleaseFixture();
    }

    physicsWorld_->GetWorld()->DestroyBody(body_);
    body_ = nullptr;
}

void RigidBody2D::ApplyWorldTransform()
{
    if (!body_ || !node_)
        return;

    // If the rigid body is parented to another rigid body, can not set the transform immediately.
    // In that case store it to PhysicsWorld2D for delayed assignment
    RigidBody2D* parentRigidBody = nullptr;
    Node* parent = node_->GetParent();
    if (parent != GetScene() && parent)
        parentRigidBody = parent->GetComponent<RigidBody2D>();

    // If body is not parented and is static or sleeping, no need to update
    if (!parentRigidBody && (!body_->IsEnabled() || body_->GetType() == b2_staticBody || !body_->IsAwake()))
        return;

    const b2Transform& transform = body_->GetTransform();
    Vector3 newWorldPosition = node_->GetWorldPosition();
    newWorldPosition.x_ = transform.p.x;
    newWorldPosition.y_ = transform.p.y;
    Quaternion newWorldRotation(transform.q.GetAngle() * M_RADTODEG, Vector3::FORWARD);

    if (parentRigidBody)
    {
        DelayedWorldTransform2D delayed;
        delayed.rigidBody_ = this;
        delayed.parentRigidBody_ = parentRigidBody;
        delayed.worldPosition_ = newWorldPosition;
        delayed.worldRotation_ = newWorldRotation;
        physicsWorld_->AddDelayedWorldTransform(delayed);
    }
    else
        ApplyWorldTransform(newWorldPosition, newWorldRotation);
}

void RigidBody2D::ApplyWorldTransform(const Vector3& newWorldPosition, const Quaternion& newWorldRotation)
{
    if (newWorldPosition != node_->GetWorldPosition() || newWorldRotation != node_->GetWorldRotation())
    {
        // Do not feed changed position back to simulation now
        physicsWorld_->SetApplyingTransforms(true);
        node_->SetWorldPosition(newWorldPosition);
        node_->SetWorldRotation(newWorldRotation);
        physicsWorld_->SetApplyingTransforms(false);
    }
}

void RigidBody2D::AddCollisionShape2D(CollisionShape2D* collisionShape)
{
    if (!collisionShape)
        return;

    WeakPtr<CollisionShape2D> collisionShapePtr(collisionShape);
    if (collisionShapes_.Contains(collisionShapePtr))
        return;

    collisionShapes_.Push(collisionShapePtr);
}

void RigidBody2D::RemoveCollisionShape2D(CollisionShape2D* collisionShape)
{
    if (!collisionShape)
        return;

    WeakPtr<CollisionShape2D> collisionShapePtr(collisionShape);
    collisionShapes_.Remove(collisionShapePtr);
}

void RigidBody2D::AddConstraint2D(Constraint2D* constraint)
{
    if (!constraint)
        return;

    WeakPtr<Constraint2D> constraintPtr(constraint);
    if (constraints_.Contains(constraintPtr))
        return;
    constraints_.Push(constraintPtr);
}

void RigidBody2D::RemoveConstraint2D(Constraint2D* constraint)
{
    if (!constraint)
        return;

    WeakPtr<Constraint2D> constraintPtr(constraint);
    constraints_.Remove(constraintPtr);
}

float RigidBody2D::GetMass() const
{
    if (!useFixtureMass_)
        return massData_.mass;
    else
        return body_ ? body_->GetMass() : 0.0f;
}

float RigidBody2D::GetInertia() const
{
    if (!useFixtureMass_)
        return massData_.I;
    else
        return body_ ? body_->GetInertia() : 0.0f;
}

Vector2 RigidBody2D::GetMassCenter() const
{
    if (!useFixtureMass_)
        return ToVector2(massData_.center);
    else
        return body_ ? ToVector2(body_->GetLocalCenter()) : Vector2::ZERO;
}

bool RigidBody2D::IsAwake() const
{
    return body_ ? body_->IsAwake() : bodyDef_.awake;
}

Vector2 RigidBody2D::GetLinearVelocity() const
{
    return ToVector2(body_ ? body_->GetLinearVelocity() : bodyDef_.linearVelocity);
}

float RigidBody2D::GetAngularVelocity() const
{
    return body_ ? body_->GetAngularVelocity() : bodyDef_.angularVelocity;
}

void RigidBody2D::OnNodeSet(Node* node)
{
    if (node)
    {
        node->AddListener(this);

        Vector<CollisionShape2D*> shapes;
        node_->GetDerivedComponents<CollisionShape2D>(shapes);

        for (Vector<CollisionShape2D*>::Iterator i = shapes.Begin(); i != shapes.End(); ++i)
        {
            (*i)->CreateFixture();
            AddCollisionShape2D(*i);
        }
    }
}

void RigidBody2D::OnSceneSet(Scene* scene)
{
    if (scene)
    {
        physicsWorld_ = scene->GetDerivedComponent<PhysicsWorld2D>();
        if (!physicsWorld_)
            physicsWorld_ = scene->CreateComponent<PhysicsWorld2D>();

        CreateBody();
        physicsWorld_->AddRigidBody(this);
    }
    else
    {
        if (physicsWorld_)
        {
            ReleaseBody();
            physicsWorld_->RemoveRigidBody(this);
            physicsWorld_.Reset();
        }
    }
}

void RigidBody2D::OnMarkedDirty(Node* node)
{
    if (physicsWorld_ && physicsWorld_->IsApplyingTransforms())
        return;

    // Physics operations are not safe from worker threads
    Scene* scene = GetScene();
    if (scene && scene->IsThreadedUpdate())
    {
        scene->DelayedMarkedDirty(this);
        return;
    }

    // Check if transform has changed from the last one set in ApplyWorldTransform()
    b2Vec2 newPosition = ToB2Vec2(node_->GetWorldPosition());
    float newAngle = node_->GetWorldRotation().RollAngle() * M_DEGTORAD;

    if (!body_)
    {
        bodyDef_.position = newPosition;
        bodyDef_.angle = newAngle;
    }
    else if (newPosition != body_->GetPosition() || newAngle != body_->GetAngle())
        body_->SetTransform(newPosition, newAngle);
}

}
