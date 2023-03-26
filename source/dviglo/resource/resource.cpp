// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "resource.h"

#include "../core/profiler.h"
#include "../core/thread.h"
#include "../io/file.h"
#include "../io/log.h"
#include "xml_element.h"

namespace dviglo
{

Resource::Resource() :
    memoryUse_(0),
    asyncLoadState_(ASYNC_DONE)
{
}

bool Resource::Load(Deserializer& source)
{
    // Because begin_load() / end_load() can be called from worker threads, where profiling would be a no-op,
    // create a type name -based profile block here
#ifdef DV_TRACY_PROFILING
    DV_PROFILE_COLOR(Load, DV_PROFILE_RESOURCE_COLOR);

    String profileBlockName("Load" + GetTypeName());
    DV_PROFILE_STR(profileBlockName.c_str(), profileBlockName.Length());
#endif

    // If we are loading synchronously in a non-main thread, behave as if async loading (for example use
    // GetTempResource() instead of GetResource() to load resource dependencies)
    SetAsyncLoadState(Thread::IsMainThread() ? ASYNC_DONE : ASYNC_LOADING);
    bool success = begin_load(source);
    if (success)
        success &= end_load();
    SetAsyncLoadState(ASYNC_DONE);

    return success;
}

bool Resource::begin_load(Deserializer& source)
{
    // This always needs to be overridden by subclasses
    return false;
}

bool Resource::end_load()
{
    // If no GPU upload step is necessary, no override is necessary
    return true;
}

bool Resource::Save(Serializer& dest) const
{
    DV_LOGERROR("Save not supported for " + GetTypeName());
    return false;
}

bool Resource::LoadFile(const String& fileName)
{
    File file;
    return file.Open(fileName, FILE_READ) && Load(file);
}

bool Resource::SaveFile(const String& fileName) const
{
    File file;
    return file.Open(fileName, FILE_WRITE) && Save(file);
}

void Resource::SetName(const String& name)
{
    name_ = name;
    nameHash_ = name;
}

void Resource::SetMemoryUse(i32 size)
{
    assert(size >= 0);
    memoryUse_ = size;
}

void Resource::ResetUseTimer()
{
    useTimer_.Reset();
}

void Resource::SetAsyncLoadState(AsyncLoadState newState)
{
    asyncLoadState_ = newState;
}

unsigned Resource::GetUseTimer()
{
    // If more references than the resource cache, return always 0 & reset the timer
    if (Refs() > 1)
    {
        useTimer_.Reset();
        return 0;
    }
    else
        return useTimer_.GetMSec(false);
}

void ResourceWithMetadata::add_metadata(const String& name, const Variant& value)
{
    bool exists;
    metadata_.Insert(MakePair(StringHash(name), value), exists);
    if (!exists)
        metadataKeys_.Push(name);
}

void ResourceWithMetadata::remove_metadata(const String& name)
{
    metadata_.Erase(name);
    metadataKeys_.Remove(name);
}

void ResourceWithMetadata::remove_all_metadata()
{
    metadata_.Clear();
    metadataKeys_.Clear();
}

const dviglo::Variant& ResourceWithMetadata::GetMetadata(const String& name) const
{
    const Variant* value = metadata_[name];
    return value ? *value : Variant::EMPTY;
}

bool ResourceWithMetadata::HasMetadata() const
{
    return !metadata_.Empty();
}

void ResourceWithMetadata::load_metadata_from_xml(const XmlElement& source)
{
    for (XmlElement elem = source.GetChild("metadata"); elem; elem = elem.GetNext("metadata"))
        add_metadata(elem.GetAttribute("name"), elem.GetVariant());
}

void ResourceWithMetadata::load_metadata_from_json(const JSONArray& array)
{
    for (const JSONValue& value : array)
        add_metadata(value.Get("name").GetString(), value.GetVariant());
}

void ResourceWithMetadata::save_metadata_to_xml(XmlElement& destination) const
{
    for (const String& metadataKey : metadataKeys_)
    {
        XmlElement elem = destination.create_child("metadata");
        elem.SetString("name", metadataKey);
        elem.SetVariant(GetMetadata(metadataKey));
    }
}

void ResourceWithMetadata::copy_metadata(const ResourceWithMetadata& source)
{
    metadata_ = source.metadata_;
    metadataKeys_ = source.metadataKeys_;
}

}
