// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../container/hash_map.h"
#include "../container/hash_set.h"
#include "../core/mutex.h"
#include "../container/ptr.h"
#include "../container/ref_counted.h"
#include "../core/thread.h"
#include "../math/string_hash.h"

namespace dviglo
{

class Resource;
class ResourceCache;

/// Queue item for background loading of a resource.
struct BackgroundLoadItem
{
    /// Resource.
    SharedPtr<Resource> resource_;
    /// Resources depended on for loading.
    HashSet<Pair<StringHash, StringHash>> dependencies_;
    /// Resources that depend on this resource's loading.
    HashSet<Pair<StringHash, StringHash>> dependents_;
    /// Whether to send failure event.
    bool sendEventOnFailure_;
};

/// Background loader of resources. Owned by the ResourceCache.
class BackgroundLoader : public RefCounted, public Thread
{
public:
    /// Construct.
    explicit BackgroundLoader(ResourceCache* owner);

    /// Destruct. Forcibly clear the load queue.
    ~BackgroundLoader() override;

    /// Resource background loading loop.
    void ThreadFunction() override;

    /// Queue loading of a resource. The name must be sanitated to ensure consistent format. Return true if queued (not a duplicate and resource was a known type).
    bool QueueResource(StringHash type, const String& name, bool sendEventOnFailure, Resource* caller);
    /// Wait and finish possible loading of a resource when being requested from the cache.
    void WaitForResource(StringHash type, StringHash nameHash);
    /// Process resources that are ready to finish.
    void FinishResources(int maxMs);

    /// Return amount of resources in the load queue.
    unsigned GetNumQueuedResources() const;

private:
    /// Finish one background loaded resource.
    void FinishBackgroundLoading(BackgroundLoadItem& item);

    /// Resource cache.
    ResourceCache* owner_;
    /// Mutex for thread-safe access to the background load queue.
    mutable Mutex backgroundLoadMutex_;
    /// Resources that are queued for background loading.
    HashMap<Pair<StringHash, StringHash>, BackgroundLoadItem> backgroundLoadQueue_;
};

}
