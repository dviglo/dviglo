// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../core/string_hash_register.h"
#include "../io/log.h"

#include <cstdio>

#include "../common/debug_new.h"

using namespace std;

namespace dviglo
{

StringHashRegister::StringHashRegister(bool threadSafe)
{
    if (threadSafe)
        mutex_ = make_unique<std::mutex>();
}


StringHashRegister::~StringHashRegister()       // NOLINT(hicpp-use-equals-default, modernize-use-equals-default)
{
    // Keep destructor here to let mutex_ destruct
}

StringHash StringHashRegister::RegisterString(const StringHash& hash, const char* string)
{
    if (mutex_)
        mutex_->lock();

    auto iter = map_.Find(hash);
    if (iter == map_.End())
    {
        map_.Populate(hash, string);
    }
    else if (iter->second_.Compare(string, false) != 0)
    {
        DV_LOGWARNINGF("StringHash collision detected! Both \"%s\" and \"%s\" have hash #%s",
            string, iter->second_.CString(), hash.ToString().CString());
    }

    if (mutex_)
        mutex_->unlock();

    return hash;
}

StringHash StringHashRegister::RegisterString(const char* string)
{
    StringHash hash(string);
    return RegisterString(hash, string);
}

String StringHashRegister::GetStringCopy(const StringHash& hash) const
{
    if (mutex_)
        mutex_->lock();

    const String copy = GetString(hash);

    if (mutex_)
        mutex_->unlock();

    return copy;
}

bool StringHashRegister::Contains(const StringHash& hash) const
{
    if (mutex_)
        mutex_->lock();

    const bool contains = map_.Contains(hash);

    if (mutex_)
        mutex_->unlock();

    return contains;
}

const String& StringHashRegister::GetString(const StringHash& hash) const
{
    auto iter = map_.Find(hash);
    return iter == map_.End() ? String::EMPTY : iter->second_;
}

}
