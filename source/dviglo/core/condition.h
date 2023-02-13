// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../dviglo_config.h"

namespace dviglo
{

/// %Condition on which a thread can wait.
class DV_API Condition
{
public:
    /// Construct.
    Condition();

    /// Destruct.
    ~Condition();

    /// Set the condition. Will be automatically reset once a waiting thread wakes up.
    void Set();

    /// Wait on the condition.
    void Wait();

private:
#ifndef _WIN32
    /// Mutex for the event, necessary for pthreads-based implementation.
    void* mutex_;
#endif
    /// Operating system specific event.
    void* event_;
};

}
