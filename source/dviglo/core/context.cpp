// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "context.h"

#include "../io/log.h"
#include "thread.h"

#include "../common/debug_new.h"

namespace dviglo
{

void EventReceiverGroup::BeginSendEvent()
{
    ++inSend_;
}

void EventReceiverGroup::EndSendEvent()
{
    assert(inSend_ > 0);
    --inSend_;

    if (inSend_ == 0 && dirty_)
    {
        /// \todo Could be optimized by erase-swap, but this keeps the receiver order
        for (i32 i = receivers_.Size() - 1; i >= 0; --i)
        {
            if (!receivers_[i])
                receivers_.Erase(i);
        }

        dirty_ = false;
    }
}

void EventReceiverGroup::Add(Object* object)
{
    if (object)
        receivers_.Push(object);
}

void EventReceiverGroup::Remove(Object* object)
{
    if (inSend_ > 0)
    {
        Vector<Object*>::Iterator i = receivers_.Find(object);
        if (i != receivers_.End())
        {
            (*i) = nullptr;
            dirty_ = true;
        }
    }
    else
        receivers_.Remove(object);
}

void RemoveNamedAttribute(HashMap<StringHash, Vector<AttributeInfo>>& attributes, StringHash objectType, const char* name)
{
    HashMap<StringHash, Vector<AttributeInfo>>::Iterator i = attributes.Find(objectType);
    if (i == attributes.End())
        return;

    Vector<AttributeInfo>& infos = i->second_;

    for (Vector<AttributeInfo>::Iterator j = infos.Begin(); j != infos.End(); ++j)
    {
        if (!j->name_.Compare(name, true))
        {
            infos.Erase(j);
            break;
        }
    }

    // If the vector became empty, erase the object type from the map
    if (infos.Empty())
        attributes.Erase(i);
}

#ifdef _DEBUG
// Проверяем, что не происходит обращения к синглтону после вызова деструктора
static bool context_destructed = false;
#endif

// Определение должно быть в cpp-файле, иначе будут проблемы в shared-версии движка в MinGW.
// Когда функция в h-файле, в exe и в dll создаются свои экземпляры объекта с разными адресами.
// https://stackoverflow.com/questions/71830151/why-singleton-in-headers-not-work-for-windows-mingw
Context& Context::get_instance()
{
    assert(!context_destructed);
    static Context instance;
    return instance;
}

Context::Context() :
    eventHandler_(nullptr)
{
#ifdef __ANDROID__
    // Always reset the random seed on Android, as the Urho3D library might not be unloaded between runs
    SetRandomSeed(1);
#endif

    // Set the main thread ID (assuming the Context is created in it)
    Thread::SetMainThread();
}

Context::~Context()
{
    // Remove subsystems that use SDL in reverse order of construction, so that Graphics can shut down SDL last
    /// \todo Context should not need to know about subsystems
    RemoveSubsystem("Renderer");
    RemoveSubsystem("Graphics");

    subsystems_.Clear();
    factories_.Clear();

    // Delete allocated event data maps
    for (Vector<VariantMap*>::Iterator i = eventDataMaps_.Begin(); i != eventDataMaps_.End(); ++i)
        delete *i;
    eventDataMaps_.Clear();

#ifdef _DEBUG
    context_destructed = true;
#endif
}

SharedPtr<Object> Context::CreateObject(StringHash objectType)
{
    HashMap<StringHash, SharedPtr<ObjectFactory>>::ConstIterator i = factories_.Find(objectType);
    if (i != factories_.End())
        return i->second_->CreateObject();
    else
        return SharedPtr<Object>();
}

void Context::RegisterFactory(ObjectFactory* factory)
{
    if (!factory)
        return;

    factories_[factory->GetType()] = factory;
}

void Context::RegisterFactory(ObjectFactory* factory, const char* category)
{
    if (!factory)
        return;

    RegisterFactory(factory);
    if (String::CStringLength(category))
        objectCategories_[category].Push(factory->GetType());
}

void Context::RegisterSubsystem(Object* object)
{
    if (!object)
        return;

    subsystems_[object->GetType()] = object;
}

void Context::RemoveSubsystem(StringHash objectType)
{
    HashMap<StringHash, SharedPtr<Object>>::Iterator i = subsystems_.Find(objectType);
    if (i != subsystems_.End())
        subsystems_.Erase(i);
}

AttributeHandle Context::RegisterAttribute(StringHash objectType, const AttributeInfo& attr)
{
    // None or pointer types can not be supported
    if (attr.type_ == VAR_NONE || attr.type_ == VAR_VOIDPTR || attr.type_ == VAR_PTR
        || attr.type_ == VAR_CUSTOM_HEAP || attr.type_ == VAR_CUSTOM_STACK)
    {
        DV_LOGWARNING("Attempt to register unsupported attribute type " + Variant::GetTypeName(attr.type_) + " to class " +
            GetTypeName(objectType));
        return AttributeHandle();
    }

    AttributeHandle handle;

    Vector<AttributeInfo>& objectAttributes = attributes_[objectType];
    objectAttributes.Push(attr);
    handle.attributeInfo_ = &objectAttributes.Back();

    if (attr.mode_ & AM_NET)
    {
        Vector<AttributeInfo>& objectNetworkAttributes = networkAttributes_[objectType];
        objectNetworkAttributes.Push(attr);
        handle.networkAttributeInfo_ = &objectNetworkAttributes.Back();
    }
    return handle;
}

void Context::RemoveAttribute(StringHash objectType, const char* name)
{
    RemoveNamedAttribute(attributes_, objectType, name);
    RemoveNamedAttribute(networkAttributes_, objectType, name);
}

void Context::RemoveAllAttributes(StringHash objectType)
{
    attributes_.Erase(objectType);
    networkAttributes_.Erase(objectType);
}

void Context::UpdateAttributeDefaultValue(StringHash objectType, const char* name, const Variant& defaultValue)
{
    AttributeInfo* info = GetAttribute(objectType, name);
    if (info)
        info->defaultValue_ = defaultValue;
}

VariantMap& Context::GetEventDataMap()
{
    unsigned nestingLevel = eventSenders_.Size();
    while (eventDataMaps_.Size() < nestingLevel + 1)
        eventDataMaps_.Push(new VariantMap());

    VariantMap& ret = *eventDataMaps_[nestingLevel];
    ret.Clear();
    return ret;
}

void Context::CopyBaseAttributes(StringHash baseType, StringHash derivedType)
{
    // Prevent endless loop if mistakenly copying attributes from same class as derived
    if (baseType == derivedType)
    {
        DV_LOGWARNING("Attempt to copy base attributes to itself for class " + GetTypeName(baseType));
        return;
    }

    const Vector<AttributeInfo>* baseAttributes = GetAttributes(baseType);
    if (baseAttributes)
    {
        for (unsigned i = 0; i < baseAttributes->Size(); ++i)
        {
            const AttributeInfo& attr = baseAttributes->At(i);
            attributes_[derivedType].Push(attr);
            if (attr.mode_ & AM_NET)
                networkAttributes_[derivedType].Push(attr);
        }
    }
}

Object* Context::GetSubsystem(StringHash type) const
{
    HashMap<StringHash, SharedPtr<Object>>::ConstIterator i = subsystems_.Find(type);
    if (i != subsystems_.End())
        return i->second_;
    else
        return nullptr;
}

const Variant& Context::GetGlobalVar(StringHash key) const
{
    VariantMap::ConstIterator i = globalVars_.Find(key);
    return i != globalVars_.End() ? i->second_ : Variant::EMPTY;
}

void Context::SetGlobalVar(StringHash key, const Variant& value)
{
    globalVars_[key] = value;
}

Object* Context::GetEventSender() const
{
    if (!eventSenders_.Empty())
        return eventSenders_.Back();
    else
        return nullptr;
}

const String& Context::GetTypeName(StringHash objectType) const
{
    // Search factories to find the hash-to-name mapping
    HashMap<StringHash, SharedPtr<ObjectFactory>>::ConstIterator i = factories_.Find(objectType);
    return i != factories_.End() ? i->second_->GetTypeName() : String::EMPTY;
}

AttributeInfo* Context::GetAttribute(StringHash objectType, const char* name)
{
    HashMap<StringHash, Vector<AttributeInfo>>::Iterator i = attributes_.Find(objectType);
    if (i == attributes_.End())
        return nullptr;

    Vector<AttributeInfo>& infos = i->second_;

    for (Vector<AttributeInfo>::Iterator j = infos.Begin(); j != infos.End(); ++j)
    {
        if (!j->name_.Compare(name, true))
            return &(*j);
    }

    return nullptr;
}

void Context::AddEventReceiver(Object* receiver, StringHash eventType)
{
    SharedPtr<EventReceiverGroup>& group = eventReceivers_[eventType];
    if (!group)
        group = new EventReceiverGroup();
    group->Add(receiver);
}

void Context::AddEventReceiver(Object* receiver, Object* sender, StringHash eventType)
{
    SharedPtr<EventReceiverGroup>& group = specificEventReceivers_[sender][eventType];
    if (!group)
        group = new EventReceiverGroup();
    group->Add(receiver);
}

void Context::RemoveEventSender(Object* sender)
{
    HashMap<Object*, HashMap<StringHash, SharedPtr<EventReceiverGroup>>>::Iterator i = specificEventReceivers_.Find(sender);
    if (i != specificEventReceivers_.End())
    {
        for (HashMap<StringHash, SharedPtr<EventReceiverGroup>>::Iterator j = i->second_.Begin(); j != i->second_.End(); ++j)
        {
            for (Vector<Object*>::Iterator k = j->second_->receivers_.Begin(); k != j->second_->receivers_.End(); ++k)
            {
                Object* receiver = *k;
                if (receiver)
                    receiver->RemoveEventSender(sender);
            }
        }
        specificEventReceivers_.Erase(i);
    }
}

void Context::RemoveEventReceiver(Object* receiver, StringHash eventType)
{
    EventReceiverGroup* group = GetEventReceivers(eventType);
    if (group)
        group->Remove(receiver);
}

void Context::RemoveEventReceiver(Object* receiver, Object* sender, StringHash eventType)
{
    EventReceiverGroup* group = GetEventReceivers(sender, eventType);
    if (group)
        group->Remove(receiver);
}

void Context::BeginSendEvent(Object* sender, StringHash eventType)
{
    eventSenders_.Push(sender);
}

void Context::EndSendEvent()
{
    eventSenders_.Pop();
}

}
