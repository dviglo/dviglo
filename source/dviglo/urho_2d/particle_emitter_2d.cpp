// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../graphics/camera.h"
#include "../graphics/material.h"
#include "../resource/resource_cache.h"
#include "../scene/scene.h"
#include "../scene/scene_events.h"
#include "particle_effect_2d.h"
#include "particle_emitter_2d.h"
#include "renderer_2d.h"
#include "sprite_2d.h"
#include "urho_2d_events.h"

#include "../common/debug_new.h"

namespace dviglo
{

extern const char* URHO2D_CATEGORY;
extern const char* blendModeNames[];

ParticleEmitter2D::ParticleEmitter2D() :
    blend_mode_(BLEND_ADDALPHA),
    numParticles_(0),
    emissionTime_(0.0f),
    emitParticleTime_(0.0f),
    boundingBoxMinPoint_(Vector3::ZERO),
    boundingBoxMaxPoint_(Vector3::ZERO),
    emitting_(true)
{
    sourceBatches_.Resize(1);
    sourceBatches_[0].owner_ = this;
}

ParticleEmitter2D::~ParticleEmitter2D() = default;

void ParticleEmitter2D::register_object()
{
    DV_CONTEXT.RegisterFactory<ParticleEmitter2D>(URHO2D_CATEGORY);

    DV_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, true, AM_DEFAULT);
    DV_COPY_BASE_ATTRIBUTES(Drawable2d);
    DV_ACCESSOR_ATTRIBUTE("Particle Effect", GetParticleEffectAttr, SetParticleEffectAttr,
        ResourceRef(ParticleEffect2D::GetTypeStatic()), AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Sprite ", GetSpriteAttr, SetSpriteAttr, ResourceRef(Sprite2D::GetTypeStatic()),
        AM_DEFAULT);
    DV_ENUM_ACCESSOR_ATTRIBUTE("Blend Mode", blend_mode, SetBlendMode, blendModeNames, BLEND_ALPHA, AM_DEFAULT);
    DV_ACCESSOR_ATTRIBUTE("Is Emitting", IsEmitting, SetEmitting, true, AM_DEFAULT);
}

void ParticleEmitter2D::OnSetEnabled()
{
    Drawable2d::OnSetEnabled();

    Scene* scene = GetScene();
    if (scene)
    {
        if (IsEnabledEffective())
            subscribe_to_event(scene, E_SCENEPOSTUPDATE, DV_HANDLER(ParticleEmitter2D, HandleScenePostUpdate));
        else
            unsubscribe_from_event(scene, E_SCENEPOSTUPDATE);
    }
}

void ParticleEmitter2D::SetEffect(ParticleEffect2D* effect)
{
    if (effect == effect_)
        return;

    effect_ = effect;
    MarkNetworkUpdate();

    if (!effect_)
        return;

    SetSprite(effect_->GetSprite());
    SetBlendMode(effect_->blend_mode());
    SetMaxParticles((unsigned)effect_->GetMaxParticles());

    emitParticleTime_ = 0.0f;
    emissionTime_ = effect_->GetDuration();
}

void ParticleEmitter2D::SetSprite(Sprite2D* sprite)
{
    if (sprite == sprite_)
        return;

    sprite_ = sprite;
    UpdateMaterial();

    MarkNetworkUpdate();
}

void ParticleEmitter2D::SetBlendMode(BlendMode blendMode)
{
    if (blendMode == blend_mode_)
        return;

    blend_mode_ = blendMode;
    UpdateMaterial();

    MarkNetworkUpdate();
}

void ParticleEmitter2D::SetMaxParticles(unsigned maxParticles)
{
    maxParticles = Max(maxParticles, 1U);

    particles_.Resize(maxParticles);
    sourceBatches_[0].vertices_.Reserve(maxParticles * 4);

    numParticles_ = Min(maxParticles, numParticles_);
}

ParticleEffect2D* ParticleEmitter2D::GetEffect() const
{
    return effect_;
}

Sprite2D* ParticleEmitter2D::GetSprite() const
{
    return sprite_;
}

void ParticleEmitter2D::SetParticleEffectAttr(const ResourceRef& value)
{
    SetEffect(DV_RES_CACHE.GetResource<ParticleEffect2D>(value.name_));
}

ResourceRef ParticleEmitter2D::GetParticleEffectAttr() const
{
    return GetResourceRef(effect_, ParticleEffect2D::GetTypeStatic());
}

void ParticleEmitter2D::SetSpriteAttr(const ResourceRef& value)
{
    Sprite2D* sprite = Sprite2D::LoadFromResourceRef(this, value);
    if (sprite)
        SetSprite(sprite);
}

void ParticleEmitter2D::SetEmitting(bool enable)
{
    if (enable == emitting_)
        return;

    emitting_ = enable;
    emitParticleTime_ = 0.0f;

    MarkNetworkUpdate();
}

ResourceRef ParticleEmitter2D::GetSpriteAttr() const
{
    return Sprite2D::SaveToResourceRef(sprite_);
}

void ParticleEmitter2D::OnSceneSet(Scene* scene)
{
    Drawable2d::OnSceneSet(scene);

    if (scene && IsEnabledEffective())
        subscribe_to_event(scene, E_SCENEPOSTUPDATE, DV_HANDLER(ParticleEmitter2D, HandleScenePostUpdate));
    else if (!scene)
        unsubscribe_from_event(E_SCENEPOSTUPDATE);
}

void ParticleEmitter2D::OnWorldBoundingBoxUpdate()
{
    boundingBox_.Clear();

    boundingBox_.Merge(boundingBoxMinPoint_);
    boundingBox_.Merge(boundingBoxMaxPoint_);

    worldBoundingBox_ = boundingBox_;
}

void ParticleEmitter2D::OnDrawOrderChanged()
{
    sourceBatches_[0].drawOrder_ = GetDrawOrder();
}

void ParticleEmitter2D::UpdateSourceBatches()
{
    if (!sourceBatchesDirty_)
        return;

    Vector<Vertex2D>& vertices = sourceBatches_[0].vertices_;
    vertices.Clear();

    if (!sprite_)
        return;

    Rect textureRect;
    if (!sprite_->GetTextureRectangle(textureRect))
        return;

    /*
    V1---------V2
    |         / |
    |       /   |
    |     /     |
    |   /       |
    | /         |
    V0---------V3
    */
    Vertex2D vertex0;
    Vertex2D vertex1;
    Vertex2D vertex2;
    Vertex2D vertex3;

    vertex0.uv_ = textureRect.min_;
    vertex1.uv_ = Vector2(textureRect.min_.x, textureRect.max_.y);
    vertex2.uv_ = textureRect.max_;
    vertex3.uv_ = Vector2(textureRect.max_.x, textureRect.min_.y);

    for (unsigned i = 0; i < numParticles_; ++i)
    {
        Particle2D& p = particles_[i];

        float rotation = -p.rotation_;
        float c = Cos(rotation);
        float s = Sin(rotation);
        float add = (c + s) * p.size_ * 0.5f;
        float sub = (c - s) * p.size_ * 0.5f;

        vertex0.position_ = Vector3(p.position_.x - sub, p.position_.y - add, p.position_.z);
        vertex1.position_ = Vector3(p.position_.x - add, p.position_.y + sub, p.position_.z);
        vertex2.position_ = Vector3(p.position_.x + sub, p.position_.y + add, p.position_.z);
        vertex3.position_ = Vector3(p.position_.x + add, p.position_.y - sub, p.position_.z);

        vertex0.color_ = vertex1.color_ = vertex2.color_ = vertex3.color_ = p.color_.ToU32();

        vertices.Push(vertex0);
        vertices.Push(vertex1);
        vertices.Push(vertex2);
        vertices.Push(vertex3);
    }

    sourceBatchesDirty_ = false;
}

void ParticleEmitter2D::UpdateMaterial()
{
    if (sprite_ && renderer_)
        sourceBatches_[0].material_ = renderer_->GetMaterial(sprite_->GetTexture(), blend_mode_);
    else
        sourceBatches_[0].material_ = nullptr;
}

void ParticleEmitter2D::HandleScenePostUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace ScenePostUpdate;
    bool hasParticles = numParticles_ > 0;
    bool emitting = emissionTime_ > 0.0f;
    float timeStep = eventData[P_TIMESTEP].GetFloat();
    Update(timeStep);

    if (emitting && emissionTime_ == 0.0f)
    {
        // Make a weak pointer to self to check for destruction during event handling
        WeakPtr<ParticleEmitter2D> self(this);
        using namespace ParticlesDuration;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_NODE] = node_;
        eventData[P_EFFECT] = effect_;
        SendEvent(E_PARTICLESDURATION, eventData); // Emitting particles stopped

        if (self.Expired())
            return;
    }
    if (hasParticles && numParticles_ == 0)
    {
        using namespace ParticlesEnd;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_NODE] = node_;
        eventData[P_EFFECT] = effect_;

        SendEvent(E_PARTICLESEND, eventData);      // All particles over
    }
}

void ParticleEmitter2D::Update(float timeStep)
{
    if (!effect_)
        return;

    Vector3 worldPosition = GetNode()->GetWorldPosition();
    float worldScale = GetNode()->GetWorldScale().x * PIXEL_SIZE;

    boundingBoxMinPoint_ = Vector3(M_INFINITY, M_INFINITY, M_INFINITY);
    boundingBoxMaxPoint_ = Vector3(-M_INFINITY, -M_INFINITY, -M_INFINITY);

    unsigned particleIndex = 0;
    while (particleIndex < numParticles_)
    {
        Particle2D& particle = particles_[particleIndex];
        if (particle.timeToLive_ > 0.0f)
        {
            UpdateParticle(particle, timeStep, worldPosition, worldScale);
            ++particleIndex;
        }
        else
        {
            if (particleIndex != numParticles_ - 1)
                particles_[particleIndex] = particles_[numParticles_ - 1];
            --numParticles_;
        }
    }

    if (emitting_ && emissionTime_ > 0.0f)
    {
        float worldAngle = GetNode()->GetWorldRotation().RollAngle();

        float timeBetweenParticles = effect_->GetParticleLifeSpan() / particles_.Size();
        emitParticleTime_ += timeStep;

        while (emitParticleTime_ > 0.0f)
        {
            if (EmitParticle(worldPosition, worldAngle, worldScale))
                UpdateParticle(particles_[numParticles_ - 1], emitParticleTime_, worldPosition, worldScale);

            emitParticleTime_ -= timeBetweenParticles;
        }

        if (emissionTime_ > 0.0f)
            emissionTime_ = Max(0.0f, emissionTime_ - timeStep);
    }

    sourceBatchesDirty_ = true;

    OnMarkedDirty(node_);
}

bool ParticleEmitter2D::EmitParticle(const Vector3& worldPosition, float worldAngle, float worldScale)
{
    if (numParticles_ >= (unsigned)effect_->GetMaxParticles() || numParticles_ >= particles_.Size())
        return false;

    float lifespan = effect_->GetParticleLifeSpan() + effect_->GetParticleLifespanVariance() * Random(-1.0f, 1.0f);
    if (lifespan <= 0.0f)
        return false;

    float invLifespan = 1.0f / lifespan;

    Particle2D& particle = particles_[numParticles_++];
    particle.timeToLive_ = lifespan;

    particle.position_.x = worldPosition.x + worldScale * effect_->GetSourcePositionVariance().x * Random(-1.0f, 1.0f);
    particle.position_.y = worldPosition.y + worldScale * effect_->GetSourcePositionVariance().y * Random(-1.0f, 1.0f);
    particle.position_.z = worldPosition.z;
    particle.startPos_.x = worldPosition.x;
    particle.startPos_.y = worldPosition.y;

    float angle = worldAngle + effect_->GetAngle() + effect_->GetAngleVariance() * Random(-1.0f, 1.0f);
    float speed = worldScale * (effect_->GetSpeed() + effect_->GetSpeedVariance() * Random(-1.0f, 1.0f));
    particle.velocity_.x = speed * Cos(angle);
    particle.velocity_.y = speed * Sin(angle);

    float maxRadius = Max(0.0f, worldScale * (effect_->GetMaxRadius() + effect_->GetMaxRadiusVariance() * Random(-1.0f, 1.0f)));
    float minRadius = Max(0.0f, worldScale * (effect_->GetMinRadius() + effect_->GetMinRadiusVariance() * Random(-1.0f, 1.0f)));
    particle.emitRadius_ = maxRadius;
    particle.emitRadiusDelta_ = (minRadius - maxRadius) * invLifespan;
    particle.emitRotation_ = worldAngle + effect_->GetAngle() + effect_->GetAngleVariance() * Random(-1.0f, 1.0f);
    particle.emitRotationDelta_ = effect_->GetRotatePerSecond() + effect_->GetRotatePerSecondVariance() * Random(-1.0f, 1.0f);
    particle.radialAcceleration_ =
        worldScale * (effect_->GetRadialAcceleration() + effect_->GetRadialAccelVariance() * Random(-1.0f, 1.0f));
    particle.tangentialAcceleration_ =
        worldScale * (effect_->GetTangentialAcceleration() + effect_->GetTangentialAccelVariance() * Random(-1.0f, 1.0f));

    float startSize =
        worldScale * Max(0.1f, effect_->GetStartParticleSize() + effect_->GetStartParticleSizeVariance() * Random(-1.0f, 1.0f));
    float finishSize =
        worldScale * Max(0.1f, effect_->GetFinishParticleSize() + effect_->GetFinishParticleSizeVariance() * Random(-1.0f, 1.0f));
    particle.size_ = startSize;
    particle.sizeDelta_ = (finishSize - startSize) * invLifespan;

    particle.color_ = effect_->GetStartColor() + effect_->GetStartColorVariance() * Random(-1.0f, 1.0f);
    Color endColor = effect_->GetFinishColor() + effect_->GetFinishColorVariance() * Random(-1.0f, 1.0f);
    particle.colorDelta_ = (endColor - particle.color_) * invLifespan;

    particle.rotation_ = worldAngle + effect_->GetRotationStart() + effect_->GetRotationStartVariance() * Random(-1.0f, 1.0f);
    float endRotation = worldAngle + effect_->GetRotationEnd() + effect_->GetRotationEndVariance() * Random(-1.0f, 1.0f);
    particle.rotationDelta_ = (endRotation - particle.rotation_) * invLifespan;

    return true;
}

void ParticleEmitter2D::UpdateParticle(Particle2D& particle, float timeStep, const Vector3& worldPosition, float worldScale)
{
    if (timeStep > particle.timeToLive_)
        timeStep = particle.timeToLive_;

    particle.timeToLive_ -= timeStep;

    if (effect_->GetEmitterType() == EMITTER_TYPE_RADIAL)
    {
        particle.emitRotation_ += particle.emitRotationDelta_ * timeStep;
        particle.emitRadius_ += particle.emitRadiusDelta_ * timeStep;

        particle.position_.x = particle.startPos_.x - Cos(particle.emitRotation_) * particle.emitRadius_;
        particle.position_.y = particle.startPos_.y + Sin(particle.emitRotation_) * particle.emitRadius_;
    }
    else
    {
        float distanceX = particle.position_.x - particle.startPos_.x;
        float distanceY = particle.position_.y - particle.startPos_.y;

        float distanceScalar = Vector2(distanceX, distanceY).Length();
        if (distanceScalar < 0.0001f)
            distanceScalar = 0.0001f;

        float radialX = distanceX / distanceScalar;
        float radialY = distanceY / distanceScalar;

        float tangentialX = radialX;
        float tangentialY = radialY;

        radialX *= particle.radialAcceleration_;
        radialY *= particle.radialAcceleration_;

        float newY = tangentialX;
        tangentialX = -tangentialY * particle.tangentialAcceleration_;
        tangentialY = newY * particle.tangentialAcceleration_;

        particle.velocity_.x += (effect_->GetGravity().x * worldScale + radialX - tangentialX) * timeStep;
        particle.velocity_.y -= (effect_->GetGravity().y * worldScale - radialY + tangentialY) * timeStep;
        particle.position_.x += particle.velocity_.x * timeStep;
        particle.position_.y += particle.velocity_.y * timeStep;
    }

    particle.size_ += particle.sizeDelta_ * timeStep;
    particle.rotation_ += particle.rotationDelta_ * timeStep;
    particle.color_ += particle.colorDelta_ * timeStep;

    float halfSize = particle.size_ * 0.5f;
    boundingBoxMinPoint_.x = Min(boundingBoxMinPoint_.x, particle.position_.x - halfSize);
    boundingBoxMinPoint_.y = Min(boundingBoxMinPoint_.y, particle.position_.y - halfSize);
    boundingBoxMinPoint_.z = Min(boundingBoxMinPoint_.z, particle.position_.z);
    boundingBoxMaxPoint_.x = Max(boundingBoxMaxPoint_.x, particle.position_.x + halfSize);
    boundingBoxMaxPoint_.y = Max(boundingBoxMaxPoint_.y, particle.position_.y + halfSize);
    boundingBoxMaxPoint_.z = Max(boundingBoxMaxPoint_.z, particle.position_.z);
}

}
