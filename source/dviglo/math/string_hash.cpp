// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../precompiled.h"

#include "../container/hash_map.h"
#include "../core/string_hash_register.h"
#include "../io/log.h"
#include "../math/string_hash.h"

#include <cstdio>

#include "../debug_new.h"

namespace dviglo
{

#ifdef URHO3D_HASH_DEBUG

// Expose map to let Visual Studio debugger access it if Urho3D is linked statically.
const StringMap* hashReverseMap = nullptr;

// Hide static global variables in functions to ensure initialization order.
static StringHashRegister& GetGlobalStringHashRegister()
{
    static StringHashRegister stringHashRegister(true /*thread safe*/ );
    hashReverseMap = &stringHashRegister.GetInternalMap();
    return stringHashRegister;
}

#endif

const StringHash StringHash::ZERO;

#ifdef URHO3D_HASH_DEBUG
StringHash::StringHash(const char* str) noexcept :
    value_(Calculate(str))
{
    dviglo::GetGlobalStringHashRegister().RegisterString(*this, str);
}
#endif

StringHash::StringHash(const String& str) noexcept :
    value_(Calculate(str.CString()))
{
#ifdef URHO3D_HASH_DEBUG
    dviglo::GetGlobalStringHashRegister().RegisterString(*this, str.CString());
#endif
}

StringHashRegister* StringHash::GetGlobalStringHashRegister()
{
#ifdef URHO3D_HASH_DEBUG
    return &dviglo::GetGlobalStringHashRegister();
#else
    return nullptr;
#endif
}

String StringHash::ToString() const
{
    char tempBuffer[CONVERSION_BUFFER_LENGTH];
    sprintf(tempBuffer, "%08X", value_);
    return String(tempBuffer);
}

String StringHash::Reverse() const
{
#ifdef URHO3D_HASH_DEBUG
    return dviglo::GetGlobalStringHashRegister().GetStringCopy(*this);
#else
    return String::EMPTY;
#endif
}

}
