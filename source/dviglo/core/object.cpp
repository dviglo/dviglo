// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "context.h"
#include "process_utils.h"
#include "thread.h"
#include "profiler.h"
#include "../io/log.h"

#include "../common/debug_new.h"


namespace dviglo
{

TypeInfo::TypeInfo(const char* typeName, const TypeInfo* baseTypeInfo) :
    type_(typeName),
    typeName_(typeName),
    baseTypeInfo_(baseTypeInfo)
{
}

TypeInfo::~TypeInfo() = default;

bool TypeInfo::IsTypeOf(StringHash type) const
{
    const TypeInfo* current = this;
    while (current)
    {
        if (current->GetType() == type)
            return true;

        current = current->GetBaseTypeInfo();
    }

    return false;
}

bool TypeInfo::IsTypeOf(const TypeInfo* typeInfo) const
{
    if (typeInfo == nullptr)
        return false;

    const TypeInfo* current = this;
    while (current)
    {
        if (current == typeInfo || current->GetType() == typeInfo->GetType())
            return true;

        current = current->GetBaseTypeInfo();
    }

    return false;
}

Object::Object() :
    blockEvents_(false)
{
    // Гарантируем, что контекст будет создан перед созданием первого объекта
    Context::get_instance();
}

Object::~Object()
{
    UnsubscribeFromAllEvents();
    DV_CONTEXT.RemoveEventSender(this);
}

void Object::OnEvent(Object* sender, StringHash eventType, VariantMap& eventData)
{
    if (blockEvents_)
        return;

    EventHandler* specific = nullptr;
    EventHandler* nonSpecific = nullptr;

    EventHandler* handler = eventHandlers_.First();
    while (handler)
    {
        if (handler->GetEventType() == eventType)
        {
            if (!handler->GetSender())
                nonSpecific = handler;
            else if (handler->GetSender() == sender)
            {
                specific = handler;
                break;
            }
        }
        handler = eventHandlers_.Next(handler);
    }

    // Specific event handlers have priority, so if found, invoke first
    if (specific)
    {
        DV_CONTEXT.SetEventHandler(specific);
        specific->Invoke(eventData);
        DV_CONTEXT.SetEventHandler(nullptr);
        return;
    }

    if (nonSpecific)
    {
        DV_CONTEXT.SetEventHandler(nonSpecific);
        nonSpecific->Invoke(eventData);
        DV_CONTEXT.SetEventHandler(nullptr);
    }
}

bool Object::IsInstanceOf(StringHash type) const
{
    return GetTypeInfo()->IsTypeOf(type);
}

bool Object::IsInstanceOf(const TypeInfo* typeInfo) const
{
    return GetTypeInfo()->IsTypeOf(typeInfo);
}

void Object::subscribe_to_event(StringHash eventType, EventHandler* handler)
{
    if (!handler)
        return;

    handler->SetSenderAndEventType(nullptr, eventType);
    // Remove old event handler first
    EventHandler* previous;
    EventHandler* oldHandler = FindSpecificEventHandler(nullptr, eventType, &previous);
    if (oldHandler)
    {
        eventHandlers_.Erase(oldHandler, previous);
        eventHandlers_.InsertFront(handler);
    }
    else
    {
        eventHandlers_.InsertFront(handler);
        DV_CONTEXT.AddEventReceiver(this, eventType);
    }
}

void Object::subscribe_to_event(Object* sender, StringHash eventType, EventHandler* handler)
{
    // If a null sender was specified, the event can not be subscribed to. Delete the handler in that case
    if (!sender || !handler)
    {
        delete handler;
        return;
    }

    handler->SetSenderAndEventType(sender, eventType);
    // Remove old event handler first
    EventHandler* previous;
    EventHandler* oldHandler = FindSpecificEventHandler(sender, eventType, &previous);
    if (oldHandler)
    {
        eventHandlers_.Erase(oldHandler, previous);
        eventHandlers_.InsertFront(handler);
    }
    else
    {
        eventHandlers_.InsertFront(handler);
        DV_CONTEXT.AddEventReceiver(this, sender, eventType);
    }
}

void Object::subscribe_to_event(StringHash eventType, const std::function<void(StringHash, VariantMap&)>& function, void* userData/*=0*/)
{
    subscribe_to_event(eventType, new EventHandler11Impl(function, userData));
}

void Object::subscribe_to_event(Object* sender, StringHash eventType, const std::function<void(StringHash, VariantMap&)>& function, void* userData/*=0*/)
{
    subscribe_to_event(sender, eventType, new EventHandler11Impl(function, userData));
}

void Object::unsubscribe_from_event(StringHash eventType)
{
    for (;;)
    {
        EventHandler* previous;
        EventHandler* handler = FindEventHandler(eventType, &previous);
        if (handler)
        {
            if (handler->GetSender())
                DV_CONTEXT.RemoveEventReceiver(this, handler->GetSender(), eventType);
            else
                DV_CONTEXT.RemoveEventReceiver(this, eventType);
            eventHandlers_.Erase(handler, previous);
        }
        else
            break;
    }
}

void Object::unsubscribe_from_event(Object* sender, StringHash eventType)
{
    if (!sender)
        return;

    EventHandler* previous;
    EventHandler* handler = FindSpecificEventHandler(sender, eventType, &previous);
    if (handler)
    {
        DV_CONTEXT.RemoveEventReceiver(this, handler->GetSender(), eventType);
        eventHandlers_.Erase(handler, previous);
    }
}

void Object::UnsubscribeFromEvents(Object* sender)
{
    if (!sender)
        return;

    for (;;)
    {
        EventHandler* previous;
        EventHandler* handler = FindSpecificEventHandler(sender, &previous);
        if (handler)
        {
            DV_CONTEXT.RemoveEventReceiver(this, handler->GetSender(), handler->GetEventType());
            eventHandlers_.Erase(handler, previous);
        }
        else
            break;
    }
}

void Object::UnsubscribeFromAllEvents()
{
    for (;;)
    {
        EventHandler* handler = eventHandlers_.First();
        if (handler)
        {
            if (handler->GetSender())
                DV_CONTEXT.RemoveEventReceiver(this, handler->GetSender(), handler->GetEventType());
            else
                DV_CONTEXT.RemoveEventReceiver(this, handler->GetEventType());
            eventHandlers_.Erase(handler);
        }
        else
            break;
    }
}

void Object::UnsubscribeFromAllEventsExcept(const Vector<StringHash>& exceptions, bool onlyUserData)
{
    EventHandler* handler = eventHandlers_.First();
    EventHandler* previous = nullptr;

    while (handler)
    {
        EventHandler* next = eventHandlers_.Next(handler);

        if ((!onlyUserData || handler->GetUserData()) && !exceptions.Contains(handler->GetEventType()))
        {
            if (handler->GetSender())
                DV_CONTEXT.RemoveEventReceiver(this, handler->GetSender(), handler->GetEventType());
            else
                DV_CONTEXT.RemoveEventReceiver(this, handler->GetEventType());

            eventHandlers_.Erase(handler, previous);
        }
        else
            previous = handler;

        handler = next;
    }
}

void Object::SendEvent(StringHash eventType)
{
    VariantMap noEventData;

    SendEvent(eventType, noEventData);
}

void Object::SendEvent(StringHash eventType, VariantMap& eventData)
{
    if (!Thread::IsMainThread())
    {
        DV_LOGERROR("Sending events is only supported from the main thread");
        return;
    }

    if (blockEvents_)
        return;

#ifdef DV_TRACY_PROFILING
    DV_PROFILE_COLOR(SendEvent, DV_PROFILE_EVENT_COLOR);

    const String& eventName = GetEventNameRegister().GetString(eventType);
    DV_PROFILE_STR(eventName.c_str(), eventName.Length());
#endif

    // Make a weak pointer to self to check for destruction during event handling
    WeakPtr<Object> self(this);
    HashSet<Object*> processed;

    DV_CONTEXT.BeginSendEvent(this, eventType);

    // Check first the specific event receivers
    // Note: group is held alive with a shared ptr, as it may get destroyed along with the sender
    SharedPtr<EventReceiverGroup> group(DV_CONTEXT.GetEventReceivers(this, eventType));
    if (group)
    {
        group->BeginSendEvent();

        const unsigned numReceivers = group->receivers_.Size();
        for (unsigned i = 0; i < numReceivers; ++i)
        {
            Object* receiver = group->receivers_[i];
            // Holes may exist if receivers removed during send
            if (!receiver)
                continue;

            receiver->OnEvent(this, eventType, eventData);

            // If self has been destroyed as a result of event handling, exit
            if (self.Expired())
            {
                group->EndSendEvent();
                DV_CONTEXT.EndSendEvent();
                return;
            }

            processed.Insert(receiver);
        }

        group->EndSendEvent();
    }

    // Then the non-specific receivers
    group = DV_CONTEXT.GetEventReceivers(eventType);
    if (group)
    {
        group->BeginSendEvent();

        if (processed.Empty())
        {
            const unsigned numReceivers = group->receivers_.Size();
            for (unsigned i = 0; i < numReceivers; ++i)
            {
                Object* receiver = group->receivers_[i];
                if (!receiver)
                    continue;

                receiver->OnEvent(this, eventType, eventData);

                if (self.Expired())
                {
                    group->EndSendEvent();
                    DV_CONTEXT.EndSendEvent();
                    return;
                }
            }
        }
        else
        {
            // If there were specific receivers, check that the event is not sent doubly to them
            const unsigned numReceivers = group->receivers_.Size();
            for (unsigned i = 0; i < numReceivers; ++i)
            {
                Object* receiver = group->receivers_[i];
                if (!receiver || processed.Contains(receiver))
                    continue;

                receiver->OnEvent(this, eventType, eventData);

                if (self.Expired())
                {
                    group->EndSendEvent();
                    DV_CONTEXT.EndSendEvent();
                    return;
                }
            }
        }

        group->EndSendEvent();
    }

    DV_CONTEXT.EndSendEvent();
}

VariantMap& Object::GetEventDataMap() const
{
    return DV_CONTEXT.GetEventDataMap();
}

const Variant& Object::GetGlobalVar(StringHash key) const
{
    return DV_CONTEXT.GetGlobalVar(key);
}

const VariantMap& Object::GetGlobalVars() const
{
    return DV_CONTEXT.GetGlobalVars();
}

void Object::SetGlobalVar(StringHash key, const Variant& value)
{
    DV_CONTEXT.SetGlobalVar(key, value);
}

Object* Object::GetEventSender() const
{
    return DV_CONTEXT.GetEventSender();
}

EventHandler* Object::GetEventHandler() const
{
    return DV_CONTEXT.GetEventHandler();
}

bool Object::HasSubscribedToEvent(StringHash eventType) const
{
    return FindEventHandler(eventType) != nullptr;
}

bool Object::HasSubscribedToEvent(Object* sender, StringHash eventType) const
{
    if (!sender)
        return false;
    else
        return FindSpecificEventHandler(sender, eventType) != nullptr;
}

const String& Object::GetCategory() const
{
    const HashMap<String, Vector<StringHash>>& objectCategories = DV_CONTEXT.GetObjectCategories();
    for (HashMap<String, Vector<StringHash>>::ConstIterator i = objectCategories.Begin(); i != objectCategories.End(); ++i)
    {
        if (i->second_.Contains(GetType()))
            return i->first_;
    }

    return String::EMPTY;
}

EventHandler* Object::FindEventHandler(StringHash eventType, EventHandler** previous) const
{
    EventHandler* handler = eventHandlers_.First();
    if (previous)
        *previous = nullptr;

    while (handler)
    {
        if (handler->GetEventType() == eventType)
            return handler;
        if (previous)
            *previous = handler;
        handler = eventHandlers_.Next(handler);
    }

    return nullptr;
}

EventHandler* Object::FindSpecificEventHandler(Object* sender, EventHandler** previous) const
{
    EventHandler* handler = eventHandlers_.First();
    if (previous)
        *previous = nullptr;

    while (handler)
    {
        if (handler->GetSender() == sender)
            return handler;
        if (previous)
            *previous = handler;
        handler = eventHandlers_.Next(handler);
    }

    return nullptr;
}

EventHandler* Object::FindSpecificEventHandler(Object* sender, StringHash eventType, EventHandler** previous) const
{
    EventHandler* handler = eventHandlers_.First();
    if (previous)
        *previous = nullptr;

    while (handler)
    {
        if (handler->GetSender() == sender && handler->GetEventType() == eventType)
            return handler;
        if (previous)
            *previous = handler;
        handler = eventHandlers_.Next(handler);
    }

    return nullptr;
}

void Object::RemoveEventSender(Object* sender)
{
    EventHandler* handler = eventHandlers_.First();
    EventHandler* previous = nullptr;

    while (handler)
    {
        if (handler->GetSender() == sender)
        {
            EventHandler* next = eventHandlers_.Next(handler);
            eventHandlers_.Erase(handler, previous);
            handler = next;
        }
        else
        {
            previous = handler;
            handler = eventHandlers_.Next(handler);
        }
    }
}

StringHashRegister& GetEventNameRegister()
{
    static StringHashRegister eventNameRegister(false /*non thread safe*/);
    return eventNameRegister;
}

}
