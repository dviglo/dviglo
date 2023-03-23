// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../core/core_events.h"
#include "../core/profiler.h"
#include "../core/work_queue.h"
#include "../io/file_system.h"
#include "../io/file_watcher.h"
#include "../io/fs_base.h"
#include "../io/log.h"
#include "../io/package_file.h"
#include "../io/path.h"
#include "background_loader.h"
#include "image.h"
#include "json_file.h"
#include "plist_file.h"
#include "resource_cache.h"
#include "resource_events.h"
#include "xml_file.h"

#include "../common/debug_new.h"

#include <cstdio>

namespace dviglo
{

static const char* checkDirs[] =
{
    "Fonts",
    "Materials",
    "Models",
    "Music",
    "Objects",
    "Particle",
    "PostProcess",
    "RenderPaths",
    "Scenes",
    "Scripts",
    "Sounds",
    "Shaders",
    "Techniques",
    "Textures",
    "UI",
    nullptr
};

static const SharedPtr<Resource> noResource;

// Проверяем, что не происходит обращения к синглтону после вызова деструктора
static bool resource_cache_destructed = false;

bool ResourceCache::is_destructed()
{
    return resource_cache_destructed;
}

// Определение должно быть в cpp-файле, иначе будут проблемы в shared-версии движка в MinGW.
// Когда функция в h-файле, в exe и в dll создаются свои экземпляры объекта с разными адресами.
// https://stackoverflow.com/questions/71830151/why-singleton-in-headers-not-work-for-windows-mingw
ResourceCache& ResourceCache::get_instance()
{
    assert(!resource_cache_destructed);
    static ResourceCache instance;
    return instance;
}

ResourceCache::ResourceCache() :
    autoReloadResources_(false),
    returnFailedResources_(false),
    searchPackagesFirst_(true),
    isRouting_(false),
    finishBackgroundResourcesMs_(5)
{
    // Register Resource library object factories
    register_resource_library();

#ifdef DV_THREADING
    // Create resource background loader. Its thread will start on the first background request
    backgroundLoader_ = new BackgroundLoader(this);
#endif

    // Subscribe BeginFrame for handling directory watchers and background loaded resource finalization
    SubscribeToEvent(E_BEGINFRAME, DV_HANDLER(ResourceCache, HandleBeginFrame));

    DV_LOGDEBUG("Singleton ResourceCache constructed");
}

ResourceCache::~ResourceCache()
{
#ifdef DV_THREADING
    // Shut down the background loader first
    backgroundLoader_.Reset();
#endif
    DV_LOGDEBUG("Singleton ResourceCache destructed");
    resource_cache_destructed = true;
}

bool ResourceCache::add_resource_dir(const String& pathName, i32 priority)
{
    assert(priority >= 0 || priority == PRIORITY_LAST);

    std::scoped_lock lock(resourceMutex_);

    if (!dir_exists(pathName))
    {
        DV_LOGERROR("Could not open directory " + pathName);
        return false;
    }

    // Convert path to absolute
    String fixedPath = sanitate_resource_dir_name(pathName);

    // Check that the same path does not already exist
    for (const String& resourceDir : resourceDirs_)
    {
        if (!resourceDir.Compare(fixedPath, false))
            return true;
    }

    if (priority >= 0 && priority < resourceDirs_.Size())
        resourceDirs_.Insert(priority, fixedPath);
    else
        resourceDirs_.Push(fixedPath);

    // If resource auto-reloading active, create a file watcher for the directory
    if (autoReloadResources_)
    {
        SharedPtr<FileWatcher> watcher(new FileWatcher());
        watcher->start_watching(fixedPath, true);
        fileWatchers_.Push(watcher);
    }

    DV_LOGINFO("Added resource path " + fixedPath);
    return true;
}

bool ResourceCache::AddPackageFile(PackageFile* package, i32 priority)
{
    assert(priority >= 0 || priority == PRIORITY_LAST);

    std::scoped_lock lock(resourceMutex_);

    // Do not add packages that failed to load
    if (!package || !package->GetNumFiles())
    {
        DV_LOGERRORF("Could not add package file %s due to load failure", package->GetName().c_str());
        return false;
    }

    if (priority >= 0 && priority < packages_.Size())
        packages_.Insert(priority, SharedPtr<PackageFile>(package));
    else
        packages_.Push(SharedPtr<PackageFile>(package));

    DV_LOGINFO("Added resource package " + package->GetName());
    return true;
}

bool ResourceCache::AddPackageFile(const String& fileName, i32 priority)
{
    assert(priority >= 0 || priority == PRIORITY_LAST);
    SharedPtr<PackageFile> package(new PackageFile());
    return package->Open(fileName) && AddPackageFile(package, priority);
}

bool ResourceCache::AddManualResource(Resource* resource)
{
    if (!resource)
    {
        DV_LOGERROR("Null manual resource");
        return false;
    }

    const String& name = resource->GetName();
    if (name.Empty())
    {
        DV_LOGERROR("Manual resource with empty name, can not add");
        return false;
    }

    resource->ResetUseTimer();
    resourceGroups_[resource->GetType()].resources_[resource->GetNameHash()] = resource;
    UpdateResourceGroup(resource->GetType());
    return true;
}

void ResourceCache::remove_resource_dir(const String& pathName)
{
    std::scoped_lock lock(resourceMutex_);

    String fixedPath = sanitate_resource_dir_name(pathName);

    for (unsigned i = 0; i < resourceDirs_.Size(); ++i)
    {
        if (!resourceDirs_[i].Compare(fixedPath, false))
        {
            resourceDirs_.Erase(i);
            // Remove the filewatcher with the matching path
            for (unsigned j = 0; j < fileWatchers_.Size(); ++j)
            {
                if (!fileWatchers_[j]->GetPath().Compare(fixedPath, false))
                {
                    fileWatchers_.Erase(j);
                    break;
                }
            }
            DV_LOGINFO("Removed resource path " + fixedPath);
            return;
        }
    }
}

void ResourceCache::remove_package_file(PackageFile* package, bool releaseResources, bool forceRelease)
{
    std::scoped_lock lock(resourceMutex_);

    for (Vector<SharedPtr<PackageFile>>::Iterator i = packages_.Begin(); i != packages_.End(); ++i)
    {
        if (*i == package)
        {
            if (releaseResources)
                ReleasePackageResources(*i, forceRelease);
            DV_LOGINFO("Removed resource package " + (*i)->GetName());
            packages_.Erase(i);
            return;
        }
    }
}

void ResourceCache::remove_package_file(const String& fileName, bool releaseResources, bool forceRelease)
{
    std::scoped_lock lock(resourceMutex_);

    // Compare the name and extension only, not the path
    String fileNameNoPath = GetFileNameAndExtension(fileName);

    for (Vector<SharedPtr<PackageFile>>::Iterator i = packages_.Begin(); i != packages_.End(); ++i)
    {
        if (!GetFileNameAndExtension((*i)->GetName()).Compare(fileNameNoPath, false))
        {
            if (releaseResources)
                ReleasePackageResources(*i, forceRelease);
            DV_LOGINFO("Removed resource package " + (*i)->GetName());
            packages_.Erase(i);
            return;
        }
    }
}

void ResourceCache::release_resource(StringHash type, const String& name, bool force)
{
    StringHash nameHash(name);
    const SharedPtr<Resource>& existingRes = FindResource(type, nameHash);
    if (!existingRes)
        return;

    // If other references exist, do not release, unless forced
    if ((existingRes.Refs() == 1 && existingRes.WeakRefs() == 0) || force)
    {
        resourceGroups_[type].resources_.Erase(nameHash);
        UpdateResourceGroup(type);
    }
}

void ResourceCache::release_resources(StringHash type, bool force)
{
    bool released = false;

    HashMap<StringHash, ResourceGroup>::Iterator i = resourceGroups_.Find(type);
    if (i != resourceGroups_.End())
    {
        for (HashMap<StringHash, SharedPtr<Resource>>::Iterator j = i->second_.resources_.Begin();
             j != i->second_.resources_.End();)
        {
            HashMap<StringHash, SharedPtr<Resource>>::Iterator current = j++;
            // If other references exist, do not release, unless forced
            if ((current->second_.Refs() == 1 && current->second_.WeakRefs() == 0) || force)
            {
                i->second_.resources_.Erase(current);
                released = true;
            }
        }
    }

    if (released)
        UpdateResourceGroup(type);
}

void ResourceCache::release_resources(StringHash type, const String& partialName, bool force)
{
    bool released = false;

    HashMap<StringHash, ResourceGroup>::Iterator i = resourceGroups_.Find(type);
    if (i != resourceGroups_.End())
    {
        for (HashMap<StringHash, SharedPtr<Resource>>::Iterator j = i->second_.resources_.Begin();
             j != i->second_.resources_.End();)
        {
            HashMap<StringHash, SharedPtr<Resource>>::Iterator current = j++;
            if (current->second_->GetName().Contains(partialName))
            {
                // If other references exist, do not release, unless forced
                if ((current->second_.Refs() == 1 && current->second_.WeakRefs() == 0) || force)
                {
                    i->second_.resources_.Erase(current);
                    released = true;
                }
            }
        }
    }

    if (released)
        UpdateResourceGroup(type);
}

void ResourceCache::release_resources(const String& partialName, bool force)
{
    // Some resources refer to others, like materials to textures. Repeat the release logic as many times as necessary to ensure
    // these get released. This is not necessary if forcing release
    bool released;
    do
    {
        released = false;

        for (HashMap<StringHash, ResourceGroup>::Iterator i = resourceGroups_.Begin(); i != resourceGroups_.End(); ++i)
        {
            for (HashMap<StringHash, SharedPtr<Resource>>::Iterator j = i->second_.resources_.Begin();
                 j != i->second_.resources_.End();)
            {
                HashMap<StringHash, SharedPtr<Resource>>::Iterator current = j++;
                if (current->second_->GetName().Contains(partialName))
                {
                    // If other references exist, do not release, unless forced
                    if ((current->second_.Refs() == 1 && current->second_.WeakRefs() == 0) || force)
                    {
                        i->second_.resources_.Erase(current);
                        released = true;
                    }
                }
            }
            if (released)
                UpdateResourceGroup(i->first_);
        }

    } while (released && !force);
}

void ResourceCache::release_all_resources(bool force)
{
    bool released;
    do
    {
        released = false;

        for (HashMap<StringHash, ResourceGroup>::Iterator i = resourceGroups_.Begin();
             i != resourceGroups_.End(); ++i)
        {
            for (HashMap<StringHash, SharedPtr<Resource>>::Iterator j = i->second_.resources_.Begin();
                 j != i->second_.resources_.End();)
            {
                HashMap<StringHash, SharedPtr<Resource>>::Iterator current = j++;
                // If other references exist, do not release, unless forced
                if ((current->second_.Refs() == 1 && current->second_.WeakRefs() == 0) || force)
                {
                    i->second_.resources_.Erase(current);
                    released = true;
                }
            }
            if (released)
                UpdateResourceGroup(i->first_);
        }

    } while (released && !force);
}

bool ResourceCache::reload_resource(Resource* resource)
{
    if (!resource)
        return false;

    resource->SendEvent(E_RELOADSTARTED);

    bool success = false;
    SharedPtr<File> file = GetFile(resource->GetName());
    if (file)
        success = resource->Load(*(file.Get()));

    if (success)
    {
        resource->ResetUseTimer();
        UpdateResourceGroup(resource->GetType());
        resource->SendEvent(E_RELOADFINISHED);
        return true;
    }

    // If reloading failed, do not remove the resource from cache, to allow for a new live edit to
    // attempt loading again
    resource->SendEvent(E_RELOADFAILED);
    return false;
}

void ResourceCache::reload_resource_with_dependencies(const String& fileName)
{
    StringHash fileNameHash(fileName);
    // If the filename is a resource we keep track of, reload it
    const SharedPtr<Resource>& resource = FindResource(fileNameHash);
    if (resource)
    {
        DV_LOGDEBUG("Reloading changed resource " + fileName);
        reload_resource(resource);
    }
    // Always perform dependency resource check for resource loaded from XML file as it could be used in inheritance
    if (!resource || GetExtension(resource->GetName()) == ".xml")
    {
        // Check if this is a dependency resource, reload dependents
        HashMap<StringHash, HashSet<StringHash>>::ConstIterator j = dependentResources_.Find(fileNameHash);
        if (j != dependentResources_.End())
        {
            // Reloading a resource may modify the dependency tracking structure. Therefore collect the
            // resources we need to reload first
            Vector<SharedPtr<Resource>> dependents;
            dependents.Reserve(j->second_.Size());

            for (HashSet<StringHash>::ConstIterator k = j->second_.Begin(); k != j->second_.End(); ++k)
            {
                const SharedPtr<Resource>& dependent = FindResource(*k);
                if (dependent)
                    dependents.Push(dependent);
            }

            for (const SharedPtr<Resource>& dependent : dependents)
            {
                DV_LOGDEBUG("Reloading resource " + dependent->GetName() + " depending on " + fileName);
                reload_resource(dependent);
            }
        }
    }
}

void ResourceCache::SetMemoryBudget(StringHash type, unsigned long long budget)
{
    resourceGroups_[type].memoryBudget_ = budget;
}

void ResourceCache::SetAutoReloadResources(bool enable)
{
    if (enable != autoReloadResources_)
    {
        if (enable)
        {
            for (const String& resourceDir : resourceDirs_)
            {
                SharedPtr<FileWatcher> watcher(new FileWatcher());
                watcher->start_watching(resourceDir, true);
                fileWatchers_.Push(watcher);
            }
        }
        else
        {
            fileWatchers_.Clear();
        }

        autoReloadResources_ = enable;
    }
}

void ResourceCache::AddResourceRouter(ResourceRouter* router, bool addAsFirst)
{
    // Check for duplicate
    for (const SharedPtr<ResourceRouter>& resourceRouter : resourceRouters_)
    {
        if (resourceRouter == router)
            return;
    }

    if (addAsFirst)
        resourceRouters_.Insert(0, SharedPtr<ResourceRouter>(router));
    else
        resourceRouters_.Push(SharedPtr<ResourceRouter>(router));
}

void ResourceCache::RemoveResourceRouter(ResourceRouter* router)
{
    for (unsigned i = 0; i < resourceRouters_.Size(); ++i)
    {
        if (resourceRouters_[i] == router)
        {
            resourceRouters_.Erase(i);
            return;
        }
    }
}

SharedPtr<File> ResourceCache::GetFile(const String& name, bool sendEventOnFailure)
{
    std::scoped_lock lock(resourceMutex_);

    String sanitatedName = sanitate_resource_name(name);

    if (!isRouting_)
    {
        isRouting_ = true;

        for (const SharedPtr<ResourceRouter>& resourceRouter : resourceRouters_)
            resourceRouter->Route(sanitatedName, RESOURCE_GETFILE);

        isRouting_ = false;
    }

    if (sanitatedName.Length())
    {
        File* file = nullptr;

        if (searchPackagesFirst_)
        {
            file = SearchPackages(sanitatedName);
            if (!file)
                file = SearchResourceDirs(sanitatedName);
        }
        else
        {
            file = SearchResourceDirs(sanitatedName);
            if (!file)
                file = SearchPackages(sanitatedName);
        }

        if (file)
            return SharedPtr<File>(file);
    }

    if (sendEventOnFailure)
    {
        if (resourceRouters_.Size() && sanitatedName.Empty() && !name.Empty())
            DV_LOGERROR("Resource request " + name + " was blocked");
        else
            DV_LOGERROR("Could not find resource " + sanitatedName);

        if (Thread::IsMainThread())
        {
            using namespace ResourceNotFound;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_RESOURCENAME] = sanitatedName.Length() ? sanitatedName : name;
            SendEvent(E_RESOURCENOTFOUND, eventData);
        }
    }

    return SharedPtr<File>();
}

Resource* ResourceCache::GetExistingResource(StringHash type, const String& name)
{
    String sanitatedName = sanitate_resource_name(name);

    if (!Thread::IsMainThread())
    {
        DV_LOGERROR("Attempted to get resource " + sanitatedName + " from outside the main thread");
        return nullptr;
    }

    // If empty name, return null pointer immediately
    if (sanitatedName.Empty())
        return nullptr;

    StringHash nameHash(sanitatedName);

    const SharedPtr<Resource>& existing = FindResource(type, nameHash);
    return existing;
}

Resource* ResourceCache::GetResource(StringHash type, const String& name, bool sendEventOnFailure)
{
    String sanitatedName = sanitate_resource_name(name);

    if (!Thread::IsMainThread())
    {
        DV_LOGERROR("Attempted to get resource " + sanitatedName + " from outside the main thread");
        return nullptr;
    }

    // If empty name, return null pointer immediately
    if (sanitatedName.Empty())
        return nullptr;

    StringHash nameHash(sanitatedName);

#ifdef DV_THREADING
    // Check if the resource is being background loaded but is now needed immediately
    backgroundLoader_->WaitForResource(type, nameHash);
#endif

    const SharedPtr<Resource>& existing = FindResource(type, nameHash);
    if (existing)
        return existing;

    SharedPtr<Resource> resource;
    // Make sure the pointer is non-null and is a Resource subclass
    resource = DynamicCast<Resource>(DV_CONTEXT.CreateObject(type));
    if (!resource)
    {
        DV_LOGERROR("Could not load unknown resource type " + String(type));

        if (sendEventOnFailure)
        {
            using namespace UnknownResourceType;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_RESOURCETYPE] = type;
            SendEvent(E_UNKNOWNRESOURCETYPE, eventData);
        }

        return nullptr;
    }

    // Attempt to load the resource
    SharedPtr<File> file = GetFile(sanitatedName, sendEventOnFailure);
    if (!file)
        return nullptr;   // Error is already logged

    DV_LOGDEBUG("Loading resource " + sanitatedName);
    resource->SetName(sanitatedName);

    if (!resource->Load(*(file.Get())))
    {
        // Error should already been logged by corresponding resource descendant class
        if (sendEventOnFailure)
        {
            using namespace LoadFailed;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_RESOURCENAME] = sanitatedName;
            SendEvent(E_LOADFAILED, eventData);
        }

        if (!returnFailedResources_)
            return nullptr;
    }

    // Store to cache
    resource->ResetUseTimer();
    resourceGroups_[type].resources_[nameHash] = resource;
    UpdateResourceGroup(type);

    return resource;
}

bool ResourceCache::BackgroundLoadResource(StringHash type, const String& name, bool sendEventOnFailure, Resource* caller)
{
#ifdef DV_THREADING
    // If empty name, fail immediately
    String sanitatedName = sanitate_resource_name(name);
    if (sanitatedName.Empty())
        return false;

    // First check if already exists as a loaded resource
    StringHash nameHash(sanitatedName);
    if (FindResource(type, nameHash) != noResource)
        return false;

    return backgroundLoader_->QueueResource(type, sanitatedName, sendEventOnFailure, caller);
#else
    // When threading not supported, fall back to synchronous loading
    return GetResource(type, name, sendEventOnFailure);
#endif
}

SharedPtr<Resource> ResourceCache::GetTempResource(StringHash type, const String& name, bool sendEventOnFailure)
{
    String sanitatedName = sanitate_resource_name(name);

    // If empty name, return null pointer immediately
    if (sanitatedName.Empty())
        return SharedPtr<Resource>();

    SharedPtr<Resource> resource;
    // Make sure the pointer is non-null and is a Resource subclass
    resource = DynamicCast<Resource>(DV_CONTEXT.CreateObject(type));
    if (!resource)
    {
        DV_LOGERROR("Could not load unknown resource type " + String(type));

        if (sendEventOnFailure)
        {
            using namespace UnknownResourceType;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_RESOURCETYPE] = type;
            SendEvent(E_UNKNOWNRESOURCETYPE, eventData);
        }

        return SharedPtr<Resource>();
    }

    // Attempt to load the resource
    SharedPtr<File> file = GetFile(sanitatedName, sendEventOnFailure);
    if (!file)
        return SharedPtr<Resource>();  // Error is already logged

    DV_LOGDEBUG("Loading temporary resource " + sanitatedName);
    resource->SetName(file->GetName());

    if (!resource->Load(*(file.Get())))
    {
        // Error should already been logged by corresponding resource descendant class
        if (sendEventOnFailure)
        {
            using namespace LoadFailed;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_RESOURCENAME] = sanitatedName;
            SendEvent(E_LOADFAILED, eventData);
        }

        return SharedPtr<Resource>();
    }

    return resource;
}

unsigned ResourceCache::GetNumBackgroundLoadResources() const
{
#ifdef DV_THREADING
    return backgroundLoader_->GetNumQueuedResources();
#else
    return 0;
#endif
}

void ResourceCache::GetResources(Vector<Resource*>& result, StringHash type) const
{
    result.Clear();
    HashMap<StringHash, ResourceGroup>::ConstIterator i = resourceGroups_.Find(type);
    if (i != resourceGroups_.End())
    {
        for (HashMap<StringHash, SharedPtr<Resource>>::ConstIterator j = i->second_.resources_.Begin();
             j != i->second_.resources_.End(); ++j)
            result.Push(j->second_);
    }
}

bool ResourceCache::Exists(const String& name) const
{
    std::scoped_lock lock(resourceMutex_);

    String sanitatedName = sanitate_resource_name(name);

    if (!isRouting_)
    {
        isRouting_ = true;

        for (const SharedPtr<ResourceRouter>& resourceRouter : resourceRouters_)
            resourceRouter->Route(sanitatedName, RESOURCE_CHECKEXISTS);

        isRouting_ = false;
    }

    if (sanitatedName.Empty())
        return false;

    for (const SharedPtr<PackageFile>& package : packages_)
    {
        if (package->Exists(sanitatedName))
            return true;
    }

    FileSystem& fileSystem = DV_FILE_SYSTEM;

    for (const String& resourceDir : resourceDirs_)
    {
        if (fileSystem.file_exists(resourceDir + sanitatedName))
            return true;
    }

    // Fallback using absolute path
    return fileSystem.file_exists(sanitatedName);
}

unsigned long long ResourceCache::GetMemoryBudget(StringHash type) const
{
    HashMap<StringHash, ResourceGroup>::ConstIterator i = resourceGroups_.Find(type);
    return i != resourceGroups_.End() ? i->second_.memoryBudget_ : 0;
}

unsigned long long ResourceCache::GetMemoryUse(StringHash type) const
{
    HashMap<StringHash, ResourceGroup>::ConstIterator i = resourceGroups_.Find(type);
    return i != resourceGroups_.End() ? i->second_.memoryUse_ : 0;
}

unsigned long long ResourceCache::GetTotalMemoryUse() const
{
    unsigned long long total = 0;
    for (HashMap<StringHash, ResourceGroup>::ConstIterator i = resourceGroups_.Begin(); i != resourceGroups_.End(); ++i)
        total += i->second_.memoryUse_;
    return total;
}

String ResourceCache::GetResourceFileName(const String& name) const
{
    FileSystem& fileSystem = DV_FILE_SYSTEM;

    for (const String& resourceDir : resourceDirs_)
    {
        if (fileSystem.file_exists(resourceDir + name))
            return resourceDir + name;
    }

    if (IsAbsolutePath(name) && fileSystem.file_exists(name))
        return name;
    else
        return String();
}

ResourceRouter* ResourceCache::GetResourceRouter(unsigned index) const
{
    return index < resourceRouters_.Size() ? resourceRouters_[index] : nullptr;
}

String ResourceCache::GetPreferredResourceDir(const String& path) const
{
    String fixedPath = add_trailing_slash(path);

    bool pathHasKnownDirs = false;
    bool parentHasKnownDirs = false;

    for (unsigned i = 0; checkDirs[i] != nullptr; ++i)
    {
        if (dir_exists(fixedPath + checkDirs[i]))
        {
            pathHasKnownDirs = true;
            break;
        }
    }
    if (!pathHasKnownDirs)
    {
        String parentPath = get_parent(fixedPath);
        for (unsigned i = 0; checkDirs[i] != nullptr; ++i)
        {
            if (dir_exists(parentPath + checkDirs[i]))
            {
                parentHasKnownDirs = true;
                break;
            }
        }
        // If path does not have known subdirectories, but the parent path has, use the parent instead
        if (parentHasKnownDirs)
            fixedPath = parentPath;
    }

    return fixedPath;
}

String ResourceCache::sanitate_resource_name(const String& name) const
{
    // Sanitate unsupported constructs from the resource name
    String sanitatedName = to_internal(name);
    sanitatedName.Replace("../", "");
    sanitatedName.Replace("./", "");

    // If the path refers to one of the resource directories, normalize the resource name
    if (resourceDirs_.Size())
    {
        String namePath = GetPath(sanitatedName);
        String exePath = DV_FILE_SYSTEM.GetProgramDir().Replaced("/./", "/");
        for (unsigned i = 0; i < resourceDirs_.Size(); ++i)
        {
            String relativeResourcePath = resourceDirs_[i];
            if (relativeResourcePath.StartsWith(exePath))
                relativeResourcePath = relativeResourcePath.Substring(exePath.Length());

            if (namePath.StartsWith(resourceDirs_[i], false))
                namePath = namePath.Substring(resourceDirs_[i].Length());
            else if (namePath.StartsWith(relativeResourcePath, false))
                namePath = namePath.Substring(relativeResourcePath.Length());
        }

        sanitatedName = namePath + GetFileNameAndExtension(sanitatedName);
    }

    return sanitatedName.Trimmed();
}

String ResourceCache::sanitate_resource_dir_name(const String& name) const
{
    String fixedPath = add_trailing_slash(name);
    if (!IsAbsolutePath(fixedPath))
        fixedPath = DV_FILE_SYSTEM.GetCurrentDir() + fixedPath;

    // Sanitate away /./ construct
    fixedPath.Replace("/./", "/");

    return fixedPath.Trimmed();
}

void ResourceCache::store_resource_dependency(Resource* resource, const String& dependency)
{
    if (!resource)
        return;

    std::scoped_lock lock(resourceMutex_);

    StringHash nameHash(resource->GetName());
    HashSet<StringHash>& dependents = dependentResources_[dependency];
    dependents.Insert(nameHash);
}

void ResourceCache::reset_dependencies(Resource* resource)
{
    if (!resource)
        return;

    std::scoped_lock lock(resourceMutex_);

    StringHash nameHash(resource->GetName());

    for (HashMap<StringHash, HashSet<StringHash>>::Iterator i = dependentResources_.Begin(); i != dependentResources_.End();)
    {
        HashSet<StringHash>& dependents = i->second_;
        dependents.Erase(nameHash);
        if (dependents.Empty())
            i = dependentResources_.Erase(i);
        else
            ++i;
    }
}

String ResourceCache::print_memory_usage() const
{
    String output = "Resource Type                 Cnt       Avg       Max    Budget     Total\n\n";
    char outputLine[256];

    unsigned totalResourceCt = 0;
    unsigned long long totalLargest = 0;
    unsigned long long totalAverage = 0;
    unsigned long long totalUse = GetTotalMemoryUse();

    for (HashMap<StringHash, ResourceGroup>::ConstIterator cit = resourceGroups_.Begin(); cit != resourceGroups_.End(); ++cit)
    {
        const unsigned resourceCt = cit->second_.resources_.Size();
        unsigned long long average = 0;
        if (resourceCt > 0)
            average = cit->second_.memoryUse_ / resourceCt;
        else
            average = 0;
        unsigned long long largest = 0;
        for (HashMap<StringHash, SharedPtr<Resource>>::ConstIterator resIt = cit->second_.resources_.Begin(); resIt != cit->second_.resources_.End(); ++resIt)
        {
            if (resIt->second_->GetMemoryUse() > largest)
                largest = resIt->second_->GetMemoryUse();
            if (largest > totalLargest)
                totalLargest = largest;
        }

        totalResourceCt += resourceCt;

        const String countString(cit->second_.resources_.Size());
        const String memUseString = GetFileSizeString(average);
        const String memMaxString = GetFileSizeString(largest);
        const String memBudgetString = GetFileSizeString(cit->second_.memoryBudget_);
        const String memTotalString = GetFileSizeString(cit->second_.memoryUse_);
        const String resTypeName = DV_CONTEXT.GetTypeName(cit->first_);

        memset(outputLine, ' ', 256);
        outputLine[255] = 0;
        sprintf(outputLine, "%-28s %4s %9s %9s %9s %9s\n", resTypeName.c_str(), countString.c_str(), memUseString.c_str(), memMaxString.c_str(), memBudgetString.c_str(), memTotalString.c_str());

        output += ((const char*)outputLine);
    }

    if (totalResourceCt > 0)
        totalAverage = totalUse / totalResourceCt;

    const String countString(totalResourceCt);
    const String memUseString = GetFileSizeString(totalAverage);
    const String memMaxString = GetFileSizeString(totalLargest);
    const String memTotalString = GetFileSizeString(totalUse);

    memset(outputLine, ' ', 256);
    outputLine[255] = 0;
    sprintf(outputLine, "%-28s %4s %9s %9s %9s %9s\n", "All", countString.c_str(), memUseString.c_str(), memMaxString.c_str(), "-", memTotalString.c_str());
    output += ((const char*)outputLine);

    return output;
}

const SharedPtr<Resource>& ResourceCache::FindResource(StringHash type, StringHash nameHash)
{
    std::scoped_lock lock(resourceMutex_);

    HashMap<StringHash, ResourceGroup>::Iterator i = resourceGroups_.Find(type);
    if (i == resourceGroups_.End())
        return noResource;
    HashMap<StringHash, SharedPtr<Resource>>::Iterator j = i->second_.resources_.Find(nameHash);
    if (j == i->second_.resources_.End())
        return noResource;

    return j->second_;
}

const SharedPtr<Resource>& ResourceCache::FindResource(StringHash nameHash)
{
    std::scoped_lock lock(resourceMutex_);

    for (HashMap<StringHash, ResourceGroup>::Iterator i = resourceGroups_.Begin(); i != resourceGroups_.End(); ++i)
    {
        HashMap<StringHash, SharedPtr<Resource>>::Iterator j = i->second_.resources_.Find(nameHash);
        if (j != i->second_.resources_.End())
            return j->second_;
    }

    return noResource;
}

void ResourceCache::ReleasePackageResources(PackageFile* package, bool force)
{
    HashSet<StringHash> affectedGroups;

    const HashMap<String, PackageEntry>& entries = package->GetEntries();
    for (HashMap<String, PackageEntry>::ConstIterator i = entries.Begin(); i != entries.End(); ++i)
    {
        StringHash nameHash(i->first_);

        // We do not know the actual resource type, so search all type containers
        for (HashMap<StringHash, ResourceGroup>::Iterator j = resourceGroups_.Begin(); j != resourceGroups_.End(); ++j)
        {
            HashMap<StringHash, SharedPtr<Resource>>::Iterator k = j->second_.resources_.Find(nameHash);
            if (k != j->second_.resources_.End())
            {
                // If other references exist, do not release, unless forced
                if ((k->second_.Refs() == 1 && k->second_.WeakRefs() == 0) || force)
                {
                    j->second_.resources_.Erase(k);
                    affectedGroups.Insert(j->first_);
                }
                break;
            }
        }
    }

    for (HashSet<StringHash>::Iterator i = affectedGroups.Begin(); i != affectedGroups.End(); ++i)
        UpdateResourceGroup(*i);
}

void ResourceCache::UpdateResourceGroup(StringHash type)
{
    HashMap<StringHash, ResourceGroup>::Iterator i = resourceGroups_.Find(type);
    if (i == resourceGroups_.End())
        return;

    for (;;)
    {
        unsigned totalSize = 0;
        unsigned oldestTimer = 0;
        HashMap<StringHash, SharedPtr<Resource>>::Iterator oldestResource = i->second_.resources_.End();

        for (HashMap<StringHash, SharedPtr<Resource>>::Iterator j = i->second_.resources_.Begin();
             j != i->second_.resources_.End(); ++j)
        {
            totalSize += j->second_->GetMemoryUse();
            unsigned useTimer = j->second_->GetUseTimer();
            if (useTimer > oldestTimer)
            {
                oldestTimer = useTimer;
                oldestResource = j;
            }
        }

        i->second_.memoryUse_ = totalSize;

        // If memory budget defined and is exceeded, remove the oldest resource and loop again
        // (resources in use always return a zero timer and can not be removed)
        if (i->second_.memoryBudget_ && i->second_.memoryUse_ > i->second_.memoryBudget_ &&
            oldestResource != i->second_.resources_.End())
        {
            DV_LOGDEBUG("Resource group " + oldestResource->second_->GetTypeName() + " over memory budget, releasing resource " +
                     oldestResource->second_->GetName());
            i->second_.resources_.Erase(oldestResource);
        }
        else
            break;
    }
}

void ResourceCache::HandleBeginFrame(StringHash eventType, VariantMap& eventData)
{
    for (unsigned i = 0; i < fileWatchers_.Size(); ++i)
    {
        String fileName;
        while (fileWatchers_[i]->GetNextChange(fileName))
        {
            reload_resource_with_dependencies(fileName);

            // Finally send a general file changed event even if the file was not a tracked resource
            using namespace FileChanged;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_FILENAME] = fileWatchers_[i]->GetPath() + fileName;
            eventData[P_RESOURCENAME] = fileName;
            SendEvent(E_FILECHANGED, eventData);
        }
    }

    // Check for background loaded resources that can be finished
#ifdef DV_THREADING
    {
        DV_PROFILE(FinishBackgroundResources);
        backgroundLoader_->FinishResources(finishBackgroundResourcesMs_);
    }
#endif
}

File* ResourceCache::SearchResourceDirs(const String& name)
{
    FileSystem& fileSystem = DV_FILE_SYSTEM;

    for (const String& resourceDir : resourceDirs_)
    {
        if (fileSystem.file_exists(resourceDir + name))
        {
            // Construct the file first with full path, then rename it to not contain the resource path,
            // so that the file's sanitatedName can be used in further GetFile() calls (for example over the network)
            File* file(new File(resourceDir + name));
            file->SetName(name);
            return file;
        }
    }

    // Fallback using absolute path
    if (fileSystem.file_exists(name))
        return new File(name);

    return nullptr;
}

File* ResourceCache::SearchPackages(const String& name)
{
    for (const SharedPtr<PackageFile>& package : packages_)
    {
        if (package->Exists(name))
            return new File(package, name);
    }

    return nullptr;
}

void register_resource_library()
{
    Image::register_object();
    JSONFile::register_object();
    PListFile::register_object();
    XmlFile::register_object();
}

}
