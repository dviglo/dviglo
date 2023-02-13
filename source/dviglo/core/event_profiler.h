// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../core/profiler.h"

namespace dviglo
{

/// Event profiling data for one block in the event profiling tree.
class DV_API EventProfilerBlock : public ProfilerBlock
{
public:
    /// Construct with the specified parent block and event ID.
    EventProfilerBlock(EventProfilerBlock* parent, StringHash eventID) :
        ProfilerBlock(parent, GetEventNameRegister().GetString(eventID).CString()),
        eventID_(eventID)
    {
    }

    /// Return child block with the specified event ID.
    EventProfilerBlock* GetChild(StringHash eventID)
    {
        for (Vector<ProfilerBlock*>::Iterator i = children_.Begin(); i != children_.End(); ++i)
        {
            auto* eventProfilerBlock = static_cast<EventProfilerBlock*>(*i);
            if (eventProfilerBlock->eventID_ == eventID)
                return eventProfilerBlock;
        }

        auto* newBlock = new EventProfilerBlock(this, eventID);
        children_.Push(newBlock);

        return newBlock;
    }

    /// Event ID.
    StringHash eventID_;
};

/// Hierarchical performance event profiler subsystem.
class DV_API EventProfiler : public Profiler
{
    DV_OBJECT(EventProfiler, Profiler);

public:
    /// Construct.
    explicit EventProfiler(Context* context);

    /// Activate the event profiler to collect information. This incurs slight performance hit on each SendEvent. By default inactive.
    static void SetActive(bool newActive) { active = newActive; }
    /// Return true if active.
    static bool IsActive() { return active; }

    /// Begin timing a profiling block based on an event ID.
    void BeginBlock(StringHash eventID)
    {
        // Profiler supports only the main thread currently
        if (!Thread::IsMainThread())
            return;

        current_ = static_cast<EventProfilerBlock*>(current_)->GetChild(eventID);
        current_->Begin();
    }

private:
    /// Profiler active. Default false.
    static bool active;
};

}
