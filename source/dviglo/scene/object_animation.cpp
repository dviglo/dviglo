// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../resource/xml_file.h"
#include "../resource/json_file.h"
#include "object_animation.h"
#include "scene_events.h"
#include "value_animation.h"
#include "value_animation_info.h"

#include "../common/debug_new.h"

namespace dviglo
{

const char* wrapModeNames[] =
{
    "Loop",
    "Once",
    "Clamp",
    nullptr
};

ObjectAnimation::ObjectAnimation()
{
}

ObjectAnimation::~ObjectAnimation() = default;

void ObjectAnimation::register_object()
{
    DV_CONTEXT.RegisterFactory<ObjectAnimation>();
}

bool ObjectAnimation::BeginLoad(Deserializer& source)
{
    XmlFile xmlFile;
    if (!xmlFile.Load(source))
        return false;

    return load_xml(xmlFile.GetRoot());
}

bool ObjectAnimation::Save(Serializer& dest) const
{
    XmlFile xmlFile;

    XmlElement rootElem = xmlFile.CreateRoot("objectanimation");
    if (!save_xml(rootElem))
        return false;

    return xmlFile.Save(dest);
}

bool ObjectAnimation::load_xml(const XmlElement& source)
{
    attributeAnimationInfos_.Clear();

    XmlElement animElem;
    animElem = source.GetChild("attributeanimation");
    while (animElem)
    {
        String name = animElem.GetAttribute("name");

        SharedPtr<ValueAnimation> animation(new ValueAnimation());
        if (!animation->load_xml(animElem))
            return false;

        String wrapModeString = animElem.GetAttribute("wrapmode");
        WrapMode wrapMode = WM_LOOP;
        for (int i = 0; i <= WM_CLAMP; ++i)
        {
            if (wrapModeString == wrapModeNames[i])
            {
                wrapMode = (WrapMode)i;
                break;
            }
        }

        float speed = animElem.GetFloat("speed");
        AddAttributeAnimation(name, animation, wrapMode, speed);

        animElem = animElem.GetNext("attributeanimation");
    }

    return true;
}

bool ObjectAnimation::save_xml(XmlElement& dest) const
{
    for (HashMap<String, SharedPtr<ValueAnimationInfo>>::ConstIterator i = attributeAnimationInfos_.Begin();
         i != attributeAnimationInfos_.End(); ++i)
    {
        XmlElement animElem = dest.create_child("attributeanimation");
        animElem.SetAttribute("name", i->first_);

        const ValueAnimationInfo* info = i->second_;
        if (!info->GetAnimation()->save_xml(animElem))
            return false;

        animElem.SetAttribute("wrapmode", wrapModeNames[info->wrap_mode()]);
        animElem.SetFloat("speed", info->GetSpeed());
    }

    return true;
}

bool ObjectAnimation::load_json(const JSONValue& source)
{
    attributeAnimationInfos_.Clear();

    JSONValue attributeAnimationsValue = source.Get("attributeanimations");
    if (attributeAnimationsValue.IsNull())
        return true;
    if (!attributeAnimationsValue.IsObject())
        return true;

    const JSONObject& attributeAnimationsObject = attributeAnimationsValue.GetObject();

    for (JSONObject::ConstIterator it = attributeAnimationsObject.Begin(); it != attributeAnimationsObject.End(); it++)
    {
        String name = it->first_;
        JSONValue value = it->second_;
        SharedPtr<ValueAnimation> animation(new ValueAnimation());
        if (!animation->load_json(value))
            return false;

        String wrapModeString = value.Get("wrapmode").GetString();
        WrapMode wrapMode = WM_LOOP;
        for (int i = 0; i <= WM_CLAMP; ++i)
        {
            if (wrapModeString == wrapModeNames[i])
            {
                wrapMode = (WrapMode)i;
                break;
            }
        }

        float speed = value.Get("speed").GetFloat();
        AddAttributeAnimation(name, animation, wrapMode, speed);
    }

    return true;
}

bool ObjectAnimation::save_json(JSONValue& dest) const
{
    JSONValue attributeAnimationsValue;

    for (HashMap<String, SharedPtr<ValueAnimationInfo>>::ConstIterator i = attributeAnimationInfos_.Begin();
         i != attributeAnimationInfos_.End(); ++i)
    {
        JSONValue animValue;
        animValue.Set("name", i->first_);

        const ValueAnimationInfo* info = i->second_;
        if (!info->GetAnimation()->save_json(animValue))
            return false;

        animValue.Set("wrapmode", wrapModeNames[info->wrap_mode()]);
        animValue.Set("speed", (float) info->GetSpeed());

        attributeAnimationsValue.Set(i->first_, animValue);
    }

    dest.Set("attributeanimations", attributeAnimationsValue);
    return true;
}

void ObjectAnimation::AddAttributeAnimation(const String& name, ValueAnimation* attributeAnimation, WrapMode wrapMode, float speed)
{
    if (!attributeAnimation)
        return;

    attributeAnimation->SetOwner(this);
    attributeAnimationInfos_[name] = new ValueAnimationInfo(attributeAnimation, wrapMode, speed);

    SendAttributeAnimationAddedEvent(name);
}

void ObjectAnimation::RemoveAttributeAnimation(const String& name)
{
    HashMap<String, SharedPtr<ValueAnimationInfo>>::Iterator i = attributeAnimationInfos_.Find(name);
    if (i != attributeAnimationInfos_.End())
    {
        SendAttributeAnimationRemovedEvent(name);

        i->second_->GetAnimation()->SetOwner(nullptr);
        attributeAnimationInfos_.Erase(i);
    }
}

void ObjectAnimation::RemoveAttributeAnimation(ValueAnimation* attributeAnimation)
{
    if (!attributeAnimation)
        return;

    for (HashMap<String, SharedPtr<ValueAnimationInfo>>::Iterator i = attributeAnimationInfos_.Begin();
         i != attributeAnimationInfos_.End(); ++i)
    {
        if (i->second_->GetAnimation() == attributeAnimation)
        {
            SendAttributeAnimationRemovedEvent(i->first_);

            attributeAnimation->SetOwner(nullptr);
            attributeAnimationInfos_.Erase(i);
            return;
        }
    }
}

ValueAnimation* ObjectAnimation::GetAttributeAnimation(const String& name) const
{
    ValueAnimationInfo* info = GetAttributeAnimationInfo(name);
    return info ? info->GetAnimation() : nullptr;
}

WrapMode ObjectAnimation::GetAttributeAnimationWrapMode(const String& name) const
{
    ValueAnimationInfo* info = GetAttributeAnimationInfo(name);
    return info ? info->wrap_mode() : WM_LOOP;
}

float ObjectAnimation::GetAttributeAnimationSpeed(const String& name) const
{
    ValueAnimationInfo* info = GetAttributeAnimationInfo(name);
    return info ? info->GetSpeed() : 1.0f;
}

ValueAnimationInfo* ObjectAnimation::GetAttributeAnimationInfo(const String& name) const
{
    HashMap<String, SharedPtr<ValueAnimationInfo>>::ConstIterator i = attributeAnimationInfos_.Find(name);
    if (i != attributeAnimationInfos_.End())
        return i->second_;
    return nullptr;
}

void ObjectAnimation::SendAttributeAnimationAddedEvent(const String& name)
{
    using namespace AttributeAnimationAdded;
    VariantMap& eventData = GetEventDataMap();
    eventData[P_OBJECTANIMATION] = this;
    eventData[P_ATTRIBUTEANIMATIONNAME] = name;
    SendEvent(E_ATTRIBUTEANIMATIONADDED, eventData);
}

void ObjectAnimation::SendAttributeAnimationRemovedEvent(const String& name)
{
    using namespace AttributeAnimationRemoved;
    VariantMap& eventData = GetEventDataMap();
    eventData[P_OBJECTANIMATION] = this;
    eventData[P_ATTRIBUTEANIMATIONNAME] = name;
    SendEvent(E_ATTRIBUTEANIMATIONREMOVED, eventData);
}

}
