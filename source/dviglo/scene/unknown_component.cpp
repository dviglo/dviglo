// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../io/deserializer.h"
#include "../io/log.h"
#include "../io/serializer.h"
#include "../resource/xml_element.h"
#include "../resource/json_value.h"
#include "unknown_component.h"

#include "../common/debug_new.h"

namespace dviglo
{

static HashMap<StringHash, String> unknownTypeToName;
static String letters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

static String GenerateNameFromType(StringHash typeHash)
{
    if (unknownTypeToName.Contains(typeHash))
        return unknownTypeToName[typeHash];

    String test;

    // Begin brute-force search
    unsigned numLetters = letters.Length();
    unsigned combinations = numLetters;
    bool found = false;

    for (unsigned i = 1; i < 6; ++i)
    {
        test.Resize(i);

        for (unsigned j = 0; j < combinations; ++j)
        {
            unsigned current = j;

            for (unsigned k = 0; k < i; ++k)
            {
                test[k] = letters[current % numLetters];
                current /= numLetters;
            }

            if (StringHash(test) == typeHash)
            {
                found = true;
                break;
            }
        }

        if (found)
            break;

        combinations *= numLetters;
    }

    unknownTypeToName[typeHash] = test;
    return test;
}

UnknownComponent::UnknownComponent() :
    useXML_(false)
{
}

void UnknownComponent::register_object()
{
    DV_CONTEXT->RegisterFactory<UnknownComponent>();
}

bool UnknownComponent::Load(Deserializer& source)
{
    useXML_ = false;
    xmlAttributes_.Clear();
    xmlAttributeInfos_.Clear();

    // Assume we are reading from a component data buffer, and the type has already been read
    unsigned dataSize = source.GetSize() - source.GetPosition();
    binaryAttributes_.Resize(dataSize);
    return dataSize ? source.Read(&binaryAttributes_[0], dataSize) == dataSize : true;
}

bool UnknownComponent::load_xml(const XmlElement& source)
{
    useXML_ = true;
    xmlAttributes_.Clear();
    xmlAttributeInfos_.Clear();
    binaryAttributes_.Clear();

    XmlElement attrElem = source.GetChild("attribute");
    while (attrElem)
    {
        AttributeInfo attr;
        attr.mode_ = AM_FILE;
        attr.name_ = attrElem.GetAttribute("name");
        attr.type_ = VAR_STRING;

        if (!attr.name_.Empty())
        {
            String attrValue = attrElem.GetAttribute("value");
            attr.defaultValue_ = String::EMPTY;
            xmlAttributeInfos_.Push(attr);
            xmlAttributes_.Push(attrValue);
        }

        attrElem = attrElem.GetNext("attribute");
    }

    // Fix up pointers to the attributes after all have been read
    for (unsigned i = 0; i < xmlAttributeInfos_.Size(); ++i)
        xmlAttributeInfos_[i].ptr_ = &xmlAttributes_[i];

    return true;
}


bool UnknownComponent::load_json(const JSONValue& source)
{
    useXML_ = true;
    xmlAttributes_.Clear();
    xmlAttributeInfos_.Clear();
    binaryAttributes_.Clear();

    JSONArray attributesArray = source.Get("attributes").GetArray();
    for (unsigned i = 0; i < attributesArray.Size(); i++)
    {
        const JSONValue& attrVal = attributesArray.At(i);

        AttributeInfo attr;
        attr.mode_ = AM_FILE;
        attr.name_ = attrVal.Get("name").GetString();
        attr.type_ = VAR_STRING;

        if (!attr.name_.Empty())
        {
            String attrValue = attrVal.Get("value").GetString();
            attr.defaultValue_ = String::EMPTY;
            xmlAttributeInfos_.Push(attr);
            xmlAttributes_.Push(attrValue);
        }
    }

    // Fix up pointers to the attributes after all have been read
    for (unsigned i = 0; i < xmlAttributeInfos_.Size(); ++i)
        xmlAttributeInfos_[i].ptr_ = &xmlAttributes_[i];

    return true;
}


bool UnknownComponent::Save(Serializer& dest) const
{
    if (useXML_)
        DV_LOGWARNING("UnknownComponent loaded in XML mode, attributes will be empty for binary save");

    // Write type and ID
    if (!dest.WriteStringHash(GetType()))
        return false;
    if (!dest.WriteU32(id_))
        return false;

    if (!binaryAttributes_.Size())
        return true;
    else
        return dest.Write(&binaryAttributes_[0], binaryAttributes_.Size()) == binaryAttributes_.Size();
}

bool UnknownComponent::save_xml(XmlElement& dest) const
{
    if (dest.IsNull())
    {
        DV_LOGERROR("Could not save " + GetTypeName() + ", null destination element");
        return false;
    }

    if (!useXML_)
        DV_LOGWARNING("UnknownComponent loaded in binary or JSON mode, attributes will be empty for XML save");

    // Write type and ID
    if (!dest.SetString("type", GetTypeName()))
        return false;
    if (!dest.SetI32("id", id_))
        return false;

    for (unsigned i = 0; i < xmlAttributeInfos_.Size(); ++i)
    {
        XmlElement attrElem = dest.create_child("attribute");
        attrElem.SetAttribute("name", xmlAttributeInfos_[i].name_);
        attrElem.SetAttribute("value", xmlAttributes_[i]);
    }

    return true;
}

bool UnknownComponent::save_json(JSONValue& dest) const
{
    if (!useXML_)
        DV_LOGWARNING("UnknownComponent loaded in binary mode, attributes will be empty for JSON save");

    // Write type and ID
    dest.Set("type", GetTypeName());
    dest.Set("id", (int) id_);

    JSONArray attributesArray;
    attributesArray.Reserve(xmlAttributeInfos_.Size());
    for (unsigned i = 0; i < xmlAttributeInfos_.Size(); ++i)
    {
        JSONValue attrVal;
        attrVal.Set("name", xmlAttributeInfos_[i].name_);
        attrVal.Set("value", xmlAttributes_[i]);
        attributesArray.Push(attrVal);
    }
    dest.Set("attributes", attributesArray);

    return true;
}

void UnknownComponent::SetTypeName(const String& typeName)
{
    typeName_ = typeName;
    typeHash_ = typeName;
}

void UnknownComponent::SetType(StringHash typeHash)
{
    typeName_ = GenerateNameFromType(typeHash);
    typeHash_ = typeHash;
}

}
