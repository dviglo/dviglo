// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include "../graphics/drawable_events.h"
#include "../scene/component.h"
#include "../scene/node.h"
#include "spriter_instance_2d.h"

#include <cmath>

namespace dviglo
{

namespace Spriter
{

SpriterInstance::SpriterInstance(Component* owner, SpriterData* spriteData) :
    owner_(owner),
    spriterData_(spriteData),
    spatialInfo_(0.f, 0.f, 0.f, 1.f, 1.f)
{
}

SpriterInstance::~SpriterInstance()
{
    Clear();

    OnSetAnimation(nullptr);
    OnSetEntity(nullptr);
}

bool SpriterInstance::SetEntity(int index)
{
    if (!spriterData_)
        return false;

    if (index < (int)spriterData_->entities_.Size())
    {
        OnSetEntity(spriterData_->entities_[index]);
        return true;
    }

    return false;
}

bool SpriterInstance::SetEntity(const String& entityName)
{
    if (!spriterData_)
        return false;

    for (Entity* entity : spriterData_->entities_)
    {
        if (entity->name_ == entityName)
        {
            OnSetEntity(entity);
            return true;
        }
    }

    return false;
}

bool SpriterInstance::SetAnimation(int index, LoopMode loopMode)
{
    if (!entity_)
        return false;

    if (index < (int)entity_->animations_.Size())
    {
        OnSetAnimation(entity_->animations_[index], loopMode);
        return true;
    }

    return false;
}

bool SpriterInstance::SetAnimation(const String& animationName, LoopMode loopMode)
{
    if (!entity_)
        return false;

    for (Animation* animation : entity_->animations_)
    {
        if (animation->name_ == animationName)
        {
            OnSetAnimation(animation, loopMode);
            return true;
        }
    }

    return false;
}

void SpriterInstance::setSpatialInfo(const SpatialInfo& spatialInfo)
{
    this->spatialInfo_ = spatialInfo;
}

void SpriterInstance::setSpatialInfo(float x, float y, float angle, float scaleX, float scaleY)
{
    spatialInfo_ = SpatialInfo(x, y, angle, scaleX, scaleY);
}

void SpriterInstance::Update(float deltaTime)
{
    if (!animation_)
        return;

    Clear();

    float lastTime = currentTime_;
    currentTime_ += deltaTime;
    if (currentTime_ > animation_->length_)
    {
        bool sendFinishEvent = false;

        if (looping_)
        {
            currentTime_ = Mod(currentTime_, animation_->length_);
            sendFinishEvent = true;
        }
        else
        {
            currentTime_ = animation_->length_;
            sendFinishEvent = lastTime != currentTime_;
        }

        if (sendFinishEvent && owner_)
        {
            Node* senderNode = owner_->GetNode();
            if (senderNode)
            {
                using namespace AnimationFinished;

                VariantMap& eventData = senderNode->GetEventDataMap();
                eventData[P_NODE] = senderNode;
                eventData[P_ANIMATION] = animation_;
                eventData[P_NAME] = animation_->name_;
                eventData[P_LOOPED] = looping_;

                senderNode->SendEvent(E_ANIMATIONFINISHED, eventData);
            }
        }
    }

    UpdateMainlineKey();
    UpdateTimelineKeys();
}

void SpriterInstance::OnSetEntity(Entity* entity)
{
    if (entity == this->entity_)
        return;

    OnSetAnimation(nullptr);

    this->entity_ = entity;
}

void SpriterInstance::OnSetAnimation(Animation* animation, LoopMode loopMode)
{
    if (animation == this->animation_)
        return;

    animation_ = animation;
    if (animation_)
    {
        if (loopMode == Default)
            looping_ = animation_->looping_;
        else if (loopMode == ForceLooped)
            looping_ = true;
        else
            looping_ = false;
    }

    currentTime_ = 0.0f;
    Clear();
}

void SpriterInstance::UpdateTimelineKeys()
{
    for (Ref* ref : mainlineKey_->boneRefs_)
    {
        BoneTimelineKey* timelineKey = (BoneTimelineKey*)GetTimelineKey(ref);
        if (ref->parent_ >= 0)
        {
            timelineKey->info_ = timelineKey->info_.UnmapFromParent(timelineKeys_[ref->parent_]->info_);
        }
        else
        {
            timelineKey->info_ = timelineKey->info_.UnmapFromParent(spatialInfo_);
        }
        timelineKeys_.Push(timelineKey);
    }

    for (Ref* ref : mainlineKey_->objectRefs_)
    {
        SpriteTimelineKey* timelineKey = (SpriteTimelineKey*)GetTimelineKey(ref);

        if (ref->parent_ >= 0)
        {
            timelineKey->info_ = timelineKey->info_.UnmapFromParent(timelineKeys_[ref->parent_]->info_);
        }
        else
        {
            timelineKey->info_ = timelineKey->info_.UnmapFromParent(spatialInfo_);
        }

        timelineKey->zIndex_ = ref->zIndex_;

        timelineKeys_.Push(timelineKey);
    }
}

void SpriterInstance::UpdateMainlineKey()
{
    const Vector<MainlineKey*>& mainlineKeys = animation_->mainlineKeys_;
    for (MainlineKey* key : mainlineKeys)
    {
        if (key->time_ <= currentTime_)
        {
            mainlineKey_ = key;
        }

        if (key->time_ >= currentTime_)
        {
            break;
        }
    }

    if (!mainlineKey_)
    {
        mainlineKey_ = mainlineKeys[0];
    }
}

TimelineKey* SpriterInstance::GetTimelineKey(Ref* ref) const
{
    Timeline* timeline = animation_->timelines_[ref->timeline_];
    TimelineKey* timelineKey = timeline->keys_[ref->key_]->Clone();
    if (timeline->keys_.Size() == 1 || timelineKey->curveType_ == INSTANT)
    {
        return timelineKey;
    }

    i32 nextTimelineKeyIndex = ref->key_ + 1;
    if (nextTimelineKeyIndex >= timeline->keys_.Size())
    {
        if (animation_->looping_)
        {
            nextTimelineKeyIndex = 0;
        }
        else
        {
            return timelineKey;
        }
    }

    TimelineKey* nextTimelineKey = timeline->keys_[nextTimelineKeyIndex];

    float nextTimelineKeyTime = nextTimelineKey->time_;
    if (nextTimelineKey->time_ < timelineKey->time_)
    {
        nextTimelineKeyTime += animation_->length_;
    }

    float t = timelineKey->GetTByCurveType(currentTime_, nextTimelineKeyTime);
    timelineKey->Interpolate(*nextTimelineKey, t);

    return timelineKey;
}

void SpriterInstance::Clear()
{
    mainlineKey_ = nullptr;

    if (!timelineKeys_.Empty())
    {
        for (const SpatialTimelineKey* key : timelineKeys_)
        {
            delete key;
        }
        timelineKeys_.Clear();
    }
}

} // namespace Spriter

} // namespace dviglo
