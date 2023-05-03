// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

/// \file

#pragma once

#include "engine.h"

#include "../core/context.h"
#include "../core/main.h"
#include "../io/log.h"


namespace dviglo
{

/// Base class for creating applications which initialize the Urho3D engine and run a main loop until exited.
class DV_API Application : public Object
{
    DV_OBJECT(Application);

public:
    SlotLogMessage log_message;

    /// Construct. Parse default engine parameters from the command line, and create the engine in an uninitialized state.
    explicit Application();

    ~Application();

    /// Setup before engine initialization. This is a chance to eg. modify the engine parameters. Call ErrorExit() to terminate without initializing the engine. Called by Application.
    virtual void Setup() { }

    /// Setup after engine initialization and before running the main loop. Call ErrorExit() to terminate without running the main loop. Called by Application.
    virtual void Start() { }

    /// Cleanup after the main loop. Called by Application.
    virtual void Stop() { }

    /// Initialize the engine and run the main loop, then return the application exit code. Catch out-of-memory exceptions while running.
    int Run();
    /// Show an error message (last log message if empty), terminate the main loop, and set failure exit code.
    void ErrorExit(const String& message = String::EMPTY);

protected:
    /// Handle log message.
    void handle_log_message(const String& message, i32 level);

    /// Engine parameters map.
    VariantMap engineParameters_;
    /// Collected startup error log messages.
    String startupErrors_;
    /// Application exit code.
    int exitCode_;
};

#define DV_DEFINE_APPLICATION_MAIN(ClassName) \
i32 run_application() \
{ \
    Context context; \
    ClassName app; \
    return app.Run(); \
} \
DV_DEFINE_MAIN(run_application())

} // namespace dviglo
