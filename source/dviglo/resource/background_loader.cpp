// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#ifdef DV_THREADING

#include "../core/context.h"
#include "../core/profiler.h"
#include "../io/log.h"
#include "background_loader.h"
#include "resource_cache.h"
#include "resource_events.h"

#include "../common/debug_new.h"

namespace dviglo
{

BackgroundLoader::BackgroundLoader(ResourceCache* owner) :
    owner_(owner)
{
}

BackgroundLoader::~BackgroundLoader()
{
    std::scoped_lock lock(background_load_mutex_);

    backgroundLoadQueue_.Clear();
}

void BackgroundLoader::ThreadFunction()
{
    DV_PROFILE_THREAD("BackgroundLoader Thread");

    while (shouldRun_)
    {
        background_load_mutex_.lock();

        // Search for a queued resource that has not been loaded yet
        HashMap<Pair<StringHash, StringHash>, BackgroundLoadItem>::Iterator i = backgroundLoadQueue_.Begin();
        while (i != backgroundLoadQueue_.End())
        {
            if (i->second_.resource_->GetAsyncLoadState() == ASYNC_QUEUED)
                break;
            else
                ++i;
        }

        if (i == backgroundLoadQueue_.End())
        {
            // No resources to load found
            background_load_mutex_.unlock();
            Time::Sleep(5);
        }
        else
        {
            BackgroundLoadItem& item = i->second_;
            Resource* resource = item.resource_;
            // We can be sure that the item is not removed from the queue as long as it is in the
            // "queued" or "loading" state
            background_load_mutex_.unlock();

            bool success = false;
            SharedPtr<File> file = owner_->GetFile(resource->GetName(), item.sendEventOnFailure_);
            if (file)
            {
                resource->SetAsyncLoadState(ASYNC_LOADING);
                success = resource->begin_load(*file);
            }

            // Process dependencies now
            // Need to lock the queue again when manipulating other entries
            Pair<StringHash, StringHash> key = MakePair(resource->GetType(), resource->GetNameHash());
            background_load_mutex_.lock();
            if (item.dependents_.Size())
            {
                for (HashSet<Pair<StringHash, StringHash>>::Iterator i = item.dependents_.Begin();
                     i != item.dependents_.End(); ++i)
                {
                    HashMap<Pair<StringHash, StringHash>, BackgroundLoadItem>::Iterator j = backgroundLoadQueue_.Find(*i);
                    if (j != backgroundLoadQueue_.End())
                        j->second_.dependencies_.Erase(key);
                }

                item.dependents_.Clear();
            }

            resource->SetAsyncLoadState(success ? ASYNC_SUCCESS : ASYNC_FAIL);
            background_load_mutex_.unlock();
        }
    }
}

bool BackgroundLoader::QueueResource(StringHash type, const String& name, bool sendEventOnFailure, Resource* caller)
{
    StringHash nameHash(name);
    Pair<StringHash, StringHash> key = MakePair(type, nameHash);

    std::scoped_lock lock(background_load_mutex_);

    // Check if already exists in the queue
    if (backgroundLoadQueue_.Find(key) != backgroundLoadQueue_.End())
        return false;

    BackgroundLoadItem& item = backgroundLoadQueue_[key];
    item.sendEventOnFailure_ = sendEventOnFailure;

    // Make sure the pointer is non-null and is a Resource subclass
    item.resource_ = DynamicCast<Resource>(DV_CONTEXT.CreateObject(type));
    if (!item.resource_)
    {
        DV_LOGERROR("Could not load unknown resource type " + String(type));

        if (sendEventOnFailure && Thread::IsMainThread())
        {
            using namespace UnknownResourceType;

            VariantMap& eventData = owner_->GetEventDataMap();
            eventData[P_RESOURCETYPE] = type;
            owner_->SendEvent(E_UNKNOWNRESOURCETYPE, eventData);
        }

        backgroundLoadQueue_.Erase(key);
        return false;
    }

    DV_LOGDEBUG("Background loading resource " + name);

    item.resource_->SetName(name);
    item.resource_->SetAsyncLoadState(ASYNC_QUEUED);

    // If this is a resource calling for the background load of more resources, mark the dependency as necessary
    if (caller)
    {
        Pair<StringHash, StringHash> callerKey = MakePair(caller->GetType(), caller->GetNameHash());
        HashMap<Pair<StringHash, StringHash>, BackgroundLoadItem>::Iterator j = backgroundLoadQueue_.Find(callerKey);
        if (j != backgroundLoadQueue_.End())
        {
            BackgroundLoadItem& callerItem = j->second_;
            item.dependents_.Insert(callerKey);
            callerItem.dependencies_.Insert(key);
        }
        else
            DV_LOGWARNING("Resource " + caller->GetName() +
                       " requested for a background loaded resource but was not in the background load queue");
    }

    // Start the background loader thread now
    if (!IsStarted())
        Run();

    return true;
}

void BackgroundLoader::WaitForResource(StringHash type, StringHash nameHash)
{
    background_load_mutex_.lock();

    // Check if the resource in question is being background loaded
    Pair<StringHash, StringHash> key = MakePair(type, nameHash);
    HashMap<Pair<StringHash, StringHash>, BackgroundLoadItem>::Iterator i = backgroundLoadQueue_.Find(key);
    if (i != backgroundLoadQueue_.End())
    {
        background_load_mutex_.unlock();

        {
            Resource* resource = i->second_.resource_;
            HiresTimer waitTimer;
            bool didWait = false;

            for (;;)
            {
                unsigned numDeps = i->second_.dependencies_.Size();
                AsyncLoadState state = resource->GetAsyncLoadState();
                if (numDeps > 0 || state == ASYNC_QUEUED || state == ASYNC_LOADING)
                {
                    didWait = true;
                    Time::Sleep(1);
                }
                else
                    break;
            }

            if (didWait)
                DV_LOGDEBUG("Waited " + String(waitTimer.GetUSec(false) / 1000) + " ms for background loaded resource " +
                         resource->GetName());
        }

        // This may take a long time and may potentially wait on other resources, so it is important we do not hold the mutex during this
        FinishBackgroundLoading(i->second_);

        background_load_mutex_.lock();
        backgroundLoadQueue_.Erase(i);
        background_load_mutex_.unlock();
    }
    else
        background_load_mutex_.unlock();
}

void BackgroundLoader::FinishResources(int maxMs)
{
    if (IsStarted())
    {
        HiresTimer timer;

        background_load_mutex_.lock();

        for (HashMap<Pair<StringHash, StringHash>, BackgroundLoadItem>::Iterator i = backgroundLoadQueue_.Begin();
             i != backgroundLoadQueue_.End();)
        {
            Resource* resource = i->second_.resource_;
            unsigned numDeps = i->second_.dependencies_.Size();
            AsyncLoadState state = resource->GetAsyncLoadState();
            if (numDeps > 0 || state == ASYNC_QUEUED || state == ASYNC_LOADING)
                ++i;
            else
            {
                // Finishing a resource may need it to wait for other resources to load, in which case we can not
                // hold on to the mutex
                background_load_mutex_.unlock();
                FinishBackgroundLoading(i->second_);
                background_load_mutex_.lock();
                i = backgroundLoadQueue_.Erase(i);
            }

            // Break when the time limit passed so that we keep sufficient FPS
            if (timer.GetUSec(false) >= maxMs * 1000LL)
                break;
        }

        background_load_mutex_.unlock();
    }
}

unsigned BackgroundLoader::GetNumQueuedResources() const
{
    std::scoped_lock lock(background_load_mutex_);
    return backgroundLoadQueue_.Size();
}

void BackgroundLoader::FinishBackgroundLoading(BackgroundLoadItem& item)
{
    Resource* resource = item.resource_;

    bool success = resource->GetAsyncLoadState() == ASYNC_SUCCESS;
    // If begin_load() phase was successful, call end_load() and get the final success/failure result
    if (success)
    {
#ifdef DV_TRACY
        DV_PROFILE_COLOR(FinishBackgroundLoading, DV_PROFILE_RESOURCE_COLOR);

        String profileBlockName("Finish" + resource->GetTypeName());
        DV_PROFILE_STR(profileBlockName.c_str(), profileBlockName.Length());
#endif

        DV_LOGDEBUG("Finishing background loaded resource " + resource->GetName());
        success = resource->end_load();
    }
    resource->SetAsyncLoadState(ASYNC_DONE);

    if (!success && item.sendEventOnFailure_)
    {
        using namespace LoadFailed;

        VariantMap& eventData = owner_->GetEventDataMap();
        eventData[P_RESOURCENAME] = resource->GetName();
        owner_->SendEvent(E_LOADFAILED, eventData);
    }

    // Store to the cache just before sending the event; use same mechanism as for manual resources
    if (success || owner_->GetReturnFailedResources())
        owner_->add_manual_resource(resource);

    // Send event, either success or failure
    {
        using namespace ResourceBackgroundLoaded;

        VariantMap& eventData = owner_->GetEventDataMap();
        eventData[P_RESOURCENAME] = resource->GetName();
        eventData[P_SUCCESS] = success;
        eventData[P_RESOURCE] = resource;
        owner_->SendEvent(E_RESOURCEBACKGROUNDLOADED, eventData);
    }
}

}

#endif
