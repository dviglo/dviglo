// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "event_profiler.h"

#include "../common/debug_new.h"

namespace dviglo
{

bool EventProfiler::active = false;

EventProfiler::EventProfiler()
{
    // FIXME: Is there a cleaner way?
    delete root_;
    current_ = root_ = new EventProfilerBlock(nullptr, "RunFrame");
    delete [] root_->name_;
    root_->name_ = new char[sizeof("RunFrame")];
    memcpy(root_->name_, "RunFrame", sizeof("RunFrame"));
}

}
