// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "hash_base.h"
#include "list_base.h"
#include "str.h"
#include "vector_base.h"

namespace dviglo
{

template <> void Swap<String>(String& first, String& second)
{
    first.Swap(second);
}

template <> void Swap<VectorBase>(VectorBase& first, VectorBase& second)
{
    first.Swap(second);
}

template <> void Swap<ListBase>(ListBase& first, ListBase& second)
{
    first.Swap(second);
}

template <> void Swap<HashBase>(HashBase& first, HashBase& second)
{
    first.Swap(second);
}

}
