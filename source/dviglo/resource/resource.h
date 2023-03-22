// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

/// \file

#pragma once

#include "../core/object.h"
#include "../core/timer.h"
#include "json_value.h"

namespace dviglo
{

class Deserializer;
class Serializer;
class XmlElement;

/// Asynchronous loading state of a resource.
enum AsyncLoadState
{
    /// No async operation in progress.
    ASYNC_DONE = 0,
    /// Queued for asynchronous loading.
    ASYNC_QUEUED = 1,
    /// In progress of calling BeginLoad() in a worker thread.
    ASYNC_LOADING = 2,
    /// BeginLoad() succeeded. EndLoad() can be called in the main thread.
    ASYNC_SUCCESS = 3,
    /// BeginLoad() failed.
    ASYNC_FAIL = 4
};

/// Base class for resources.
class DV_API Resource : public Object
{
    DV_OBJECT(Resource, Object);

public:
    /// Construct.
    explicit Resource();

    /// Load resource synchronously. Call both BeginLoad() & EndLoad() and return true if both succeeded.
    bool Load(Deserializer& source);
    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    virtual bool EndLoad();
    /// Save resource. Return true if successful.
    virtual bool Save(Serializer& dest) const;

    /// Load resource from file.
    bool LoadFile(const String& fileName);
    /// Save resource to file.
    virtual bool SaveFile(const String& fileName) const;

    /// Set name.
    void SetName(const String& name);
    /// Set memory use in bytes, possibly approximate.
    void SetMemoryUse(i32 size);
    /// Reset last used timer.
    void ResetUseTimer();
    /// Set the asynchronous loading state. Called by ResourceCache. Resources in the middle of asynchronous loading are not normally returned to user.
    void SetAsyncLoadState(AsyncLoadState newState);

    /// Return name.
    const String& GetName() const { return name_; }

    /// Return name hash.
    StringHash GetNameHash() const { return nameHash_; }

    /// Return memory use in bytes, possibly approximate.
    i32 GetMemoryUse() const { return memoryUse_; }

    /// Return time since last use in milliseconds. If referred to elsewhere than in the resource cache, returns always zero.
    unsigned GetUseTimer();

    /// Return the asynchronous loading state.
    AsyncLoadState GetAsyncLoadState() const { return asyncLoadState_; }

private:
    /// Name.
    String name_;
    /// Name hash.
    StringHash nameHash_;
    /// Last used timer.
    Timer useTimer_;
    /// Memory use in bytes.
    i32 memoryUse_;
    /// Asynchronous loading state.
    AsyncLoadState asyncLoadState_;
};

/// Base class for resources that support arbitrary metadata stored. Metadata serialization shall be implemented in derived classes.
class DV_API ResourceWithMetadata : public Resource
{
    DV_OBJECT(ResourceWithMetadata, Resource);

public:
    /// Construct.
    explicit ResourceWithMetadata() { }

    /// Add new metadata variable or overwrite old value.
    void AddMetadata(const String& name, const Variant& value);
    /// Remove metadata variable.
    void remove_metadata(const String& name);
    /// Remove all metadata variables.
    void RemoveAllMetadata();
    /// Return metadata variable.
    const Variant& GetMetadata(const String& name) const;
    /// Return whether the resource has metadata.
    bool HasMetadata() const;

protected:
    /// Load metadata from <metadata> children of XML element.
    void load_metadata_from_xml(const XmlElement& source);
    /// Load metadata from JSON array.
    void load_metadata_from_json(const JSONArray& array);
    /// Save as <metadata> children of XML element.
    void save_metadata_to_xml(XmlElement& destination) const;
    /// Copy metadata from another resource.
    void copy_metadata(const ResourceWithMetadata& source);

private:
    /// Animation metadata variables.
    VariantMap metadata_;
    /// Animation metadata keys.
    StringVector metadataKeys_;
};

inline const String& GetResourceName(Resource* resource)
{
    return resource ? resource->GetName() : String::EMPTY;
}

inline StringHash GetResourceType(Resource* resource, StringHash defaultType)
{
    return resource ? resource->GetType() : defaultType;
}

inline ResourceRef GetResourceRef(Resource* resource, StringHash defaultType)
{
    return ResourceRef(GetResourceType(resource, defaultType), GetResourceName(resource));
}

template <class T> Vector<String> GetResourceNames(const Vector<SharedPtr<T>>& resources)
{
    Vector<String> ret(resources.Size());
    for (i32 i = 0; i < resources.Size(); ++i)
        ret[i] = GetResourceName(resources[i]);

    return ret;
}

template <class T> ResourceRefList GetResourceRefList(const Vector<SharedPtr<T>>& resources)
{
    return ResourceRefList(T::GetTypeStatic(), GetResourceNames(resources));
}

}
