// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../container/array_ptr.h"
#include "../core/profiler.h"
#include "../core/context.h"
#include "../io/deserializer.h"
#include "../io/log.h"
#include "../io/memory_buffer.h"
#include "../resource/json_file.h"
#include "../resource/resource_cache.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include "../common/debug_new.h"

using namespace rapidjson;

namespace dviglo
{

JSONFile::JSONFile(Context* context) :
    Resource(context)
{
}

JSONFile::~JSONFile() = default;

void JSONFile::RegisterObject(Context* context)
{
    context->RegisterFactory<JSONFile>();
}

// Convert rapidjson value to JSON value.
static void ToJSONValue(JSONValue& jsonValue, const rapidjson::Value& rapidjsonValue)
{
    switch (rapidjsonValue.GetType())
    {
    case kNullType:
        // Reset to null type
        jsonValue.SetType(JSON_NULL);
        break;

    case kFalseType:
        jsonValue = false;
        break;

    case kTrueType:
        jsonValue = true;
        break;

    case kNumberType:
        if (rapidjsonValue.IsInt())
            jsonValue = rapidjsonValue.GetInt();
        else if (rapidjsonValue.IsUint())
            jsonValue = rapidjsonValue.GetUint();
        else
            jsonValue = rapidjsonValue.GetDouble();
        break;

    case kStringType:
        jsonValue = rapidjsonValue.GetString();
        break;

    case kArrayType:
        {
            jsonValue.Resize(rapidjsonValue.Size());
            for (unsigned i = 0; i < rapidjsonValue.Size(); ++i)
            {
                ToJSONValue(jsonValue[i], rapidjsonValue[i]);
            }
        }
        break;

    case kObjectType:
        {
            jsonValue.SetType(JSON_OBJECT);
            for (rapidjson::Value::ConstMemberIterator i = rapidjsonValue.MemberBegin(); i != rapidjsonValue.MemberEnd(); ++i)
            {
                JSONValue& value = jsonValue[String(i->name.GetString())];
                ToJSONValue(value, i->value);
            }
        }
        break;

    default:
        break;
    }
}

bool JSONFile::BeginLoad(Deserializer& source)
{
    unsigned dataSize = source.GetSize();
    if (!dataSize && !source.GetName().Empty())
    {
        DV_LOGERROR("Zero sized JSON data in " + source.GetName());
        return false;
    }

    SharedArrayPtr<char> buffer(new char[dataSize + 1]);
    if (source.Read(buffer.Get(), dataSize) != dataSize)
        return false;
    buffer[dataSize] = '\0';

    rapidjson::Document document;
    if (document.Parse<kParseCommentsFlag | kParseTrailingCommasFlag>(buffer).HasParseError())
    {
        DV_LOGERROR("Could not parse JSON data from " + source.GetName());
        return false;
    }

    ToJSONValue(root_, document);

    SetMemoryUse(dataSize);

    return true;
}

static void ToRapidjsonValue(rapidjson::Value& rapidjsonValue, const JSONValue& jsonValue, rapidjson::MemoryPoolAllocator<>& allocator)
{
    switch (jsonValue.GetValueType())
    {
    case JSON_NULL:
        rapidjsonValue.SetNull();
        break;

    case JSON_BOOL:
        rapidjsonValue.SetBool(jsonValue.GetBool());
        break;

    case JSON_NUMBER:
        {
            switch (jsonValue.GetNumberType())
            {
            case JSONNT_INT:
                rapidjsonValue.SetInt(jsonValue.GetI32());
                break;

            case JSONNT_UINT:
                rapidjsonValue.SetUint(jsonValue.GetU32());
                break;

            default:
                rapidjsonValue.SetDouble(jsonValue.GetDouble());
                break;
            }
        }
        break;

    case JSON_STRING:
        rapidjsonValue.SetString(jsonValue.GetCString(), allocator);
        break;

    case JSON_ARRAY:
        {
            const JSONArray& jsonArray = jsonValue.GetArray();

            rapidjsonValue.SetArray();
            rapidjsonValue.Reserve(jsonArray.Size(), allocator);

            for (const JSONValue& jsonValue : jsonArray)
            {
                rapidjson::Value value;
                ToRapidjsonValue(value, jsonValue, allocator);
                rapidjsonValue.PushBack(value, allocator);
            }
        }
        break;

    case JSON_OBJECT:
        {
            const JSONObject& jsonObject = jsonValue.GetObject();

            rapidjsonValue.SetObject();
            for (JSONObject::ConstIterator i = jsonObject.Begin(); i != jsonObject.End(); ++i)
            {
                const char* name = i->first_.CString();
                rapidjson::Value value;
                ToRapidjsonValue(value, i->second_, allocator);
                rapidjsonValue.AddMember(StringRef(name), value, allocator);
            }
        }
        break;

    default:
        break;
    }
}

bool JSONFile::Save(Serializer& dest) const
{
    return Save(dest, "\t");
}

bool JSONFile::Save(Serializer& dest, const String& indendation) const
{
    rapidjson::Document document;
    ToRapidjsonValue(document, root_, document.GetAllocator());

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    writer.SetIndent(!indendation.Empty() ? indendation.Front() : '\0', indendation.Length());

    document.Accept(writer);
    auto size = (unsigned)buffer.GetSize();
    return dest.Write(buffer.GetString(), size) == size;
}

bool JSONFile::FromString(const String & source)
{
    if (source.Empty())
        return false;

    MemoryBuffer buffer(source.CString(), source.Length());
    return Load(buffer);
}

String JSONFile::ToString(const String& indendation) const
{
    rapidjson::Document document;
    ToRapidjsonValue(document, root_, document.GetAllocator());

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    writer.SetIndent(!indendation.Empty() ? indendation.Front() : '\0', indendation.Length());

    document.Accept(writer);
    return buffer.GetString();
}

}
