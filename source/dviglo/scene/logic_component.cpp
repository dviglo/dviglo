// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../io/log.h"
#if defined(DV_BULLET) || defined(DV_BOX2D)
#include "../physics/physics_events.h"
#endif
#include "logic_component.h"
#include "scene.h"
#include "scene_events.h"

using namespace std;


namespace dviglo
{

LogicComponent::LogicComponent() :
    updateEventMask_(LogicComponentEvents::All),
    currentEventMask_(LogicComponentEvents::None),
    delayedStartCalled_(false)
{
}

LogicComponent::~LogicComponent() = default;

void LogicComponent::OnSetEnabled()
{
    UpdateEventSubscription();
}

void LogicComponent::Update(float timeStep)
{
}

void LogicComponent::PostUpdate(float timeStep)
{
}

void LogicComponent::FixedUpdate(float timeStep)
{
}

void LogicComponent::FixedPostUpdate(float timeStep)
{
}

void LogicComponent::SetUpdateEventMask(LogicComponentEvents mask)
{
    if (updateEventMask_ != mask)
    {
        updateEventMask_ = mask;
        UpdateEventSubscription();
    }
}

void LogicComponent::OnNodeSet(Node* node)
{
    if (node)
    {
        // Execute the user-defined start function
        Start();
    }
    else
    {
        // We are being detached from a node: execute user-defined stop function and prepare for destruction
        Stop();
    }
}

void LogicComponent::OnSceneSet(Scene* scene)
{
    if (scene)
        UpdateEventSubscription();
    else
    {
        unsubscribe_from_event(E_SCENEUPDATE);
        unsubscribe_from_event(E_SCENEPOSTUPDATE);
        scene_update.disconnect();
        scene_post_update.disconnect();
#if defined(DV_BULLET) || defined(DV_BOX2D)
        unsubscribe_from_event(E_PHYSICSPRESTEP);
        unsubscribe_from_event(E_PHYSICSPOSTSTEP);
#endif
        currentEventMask_ = LogicComponentEvents::None;
    }
}

void LogicComponent::UpdateEventSubscription()
{
    Scene* scene = GetScene();
    if (!scene)
        return;

    bool enabled = IsEnabledEffective();

    bool needUpdate = enabled && (!!(updateEventMask_ & LogicComponentEvents::Update) || !delayedStartCalled_);
    if (needUpdate && !(currentEventMask_ & LogicComponentEvents::Update))
    {
        scene_update.connect(scene->scene_update, bind(&LogicComponent::handle_scene_update, this, placeholders::_1, placeholders::_2));
        currentEventMask_ |= LogicComponentEvents::Update;
    }
    else if (!needUpdate && !!(currentEventMask_ & LogicComponentEvents::Update))
    {
        unsubscribe_from_event(scene, E_SCENEUPDATE);
        scene_update.disconnect();
        currentEventMask_ &= ~LogicComponentEvents::Update;
    }

    bool needPostUpdate = enabled && !!(updateEventMask_ & LogicComponentEvents::PostUpdate);
    if (needPostUpdate && !(currentEventMask_ & LogicComponentEvents::PostUpdate))
    {
        scene_post_update.connect(scene->scene_post_update, bind(&LogicComponent::handle_scene_post_update, this, placeholders::_1, placeholders::_2));
        currentEventMask_ |= LogicComponentEvents::PostUpdate;
    }
    else if (!needPostUpdate && !!(currentEventMask_ & LogicComponentEvents::PostUpdate))
    {
        unsubscribe_from_event(scene, E_SCENEPOSTUPDATE);
        scene_post_update.disconnect();
        currentEventMask_ &= ~LogicComponentEvents::PostUpdate;
    }

#if defined(DV_BULLET) || defined(DV_BOX2D)
    Component* world = GetFixedUpdateSource();
    if (!world)
        return;

    bool needFixedUpdate = enabled && !!(updateEventMask_ & LogicComponentEvents::FixedUpdate);
    if (needFixedUpdate && !(currentEventMask_ & LogicComponentEvents::FixedUpdate))
    {
        subscribe_to_event(world, E_PHYSICSPRESTEP, DV_HANDLER(LogicComponent, HandlePhysicsPreStep));
        currentEventMask_ |= LogicComponentEvents::FixedUpdate;
    }
    else if (!needFixedUpdate && !!(currentEventMask_ & LogicComponentEvents::FixedUpdate))
    {
        unsubscribe_from_event(world, E_PHYSICSPRESTEP);
        currentEventMask_ &= ~LogicComponentEvents::FixedUpdate;
    }

    bool needFixedPostUpdate = enabled && !!(updateEventMask_ & LogicComponentEvents::FixedPostUpdate);
    if (needFixedPostUpdate && !(currentEventMask_ & LogicComponentEvents::FixedPostUpdate))
    {
        subscribe_to_event(world, E_PHYSICSPOSTSTEP, DV_HANDLER(LogicComponent, HandlePhysicsPostStep));
        currentEventMask_ |= LogicComponentEvents::FixedPostUpdate;
    }
    else if (!needFixedPostUpdate && !!(currentEventMask_ & LogicComponentEvents::FixedPostUpdate))
    {
        unsubscribe_from_event(world, E_PHYSICSPOSTSTEP);
        currentEventMask_ &= ~LogicComponentEvents::FixedPostUpdate;
    }
#endif
}

void LogicComponent::handle_scene_update(Scene* scene, float time_step)
{
    // Execute user-defined delayed start function before first update
    if (!delayedStartCalled_)
    {
        DelayedStart();
        delayedStartCalled_ = true;

        // If did not need actual update events, unsubscribe now
        if (!(updateEventMask_ & LogicComponentEvents::Update))
        {
            unsubscribe_from_event(GetScene(), E_SCENEUPDATE);
            currentEventMask_ &= ~LogicComponentEvents::Update;
            return;
        }
    }

    // Then execute user-defined update function
    Update(time_step);
}

void LogicComponent::handle_scene_post_update(Scene* scene, float time_step)
{
    PostUpdate(time_step);
}

#if defined(DV_BULLET) || defined(DV_BOX2D)

void LogicComponent::HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData)
{
    using namespace PhysicsPreStep;

    // Execute user-defined delayed start function before first fixed update if not called yet
    if (!delayedStartCalled_)
    {
        DelayedStart();
        delayedStartCalled_ = true;
    }

    // Execute user-defined fixed update function
    FixedUpdate(eventData[P_TIMESTEP].GetFloat());
}

void LogicComponent::HandlePhysicsPostStep(StringHash eventType, VariantMap& eventData)
{
    using namespace PhysicsPostStep;

    // Execute user-defined fixed post-update function
    FixedPostUpdate(eventData[P_TIMESTEP].GetFloat());
}

#endif

}
