// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "application.h"
#include "../io/io_events.h"
#include "../io/log.h"

#include "../core/sdl_helper.h"

#include "../common/debug_new.h"

namespace dviglo
{

Application::Application() :
    exitCode_(EXIT_SUCCESS)
{
    engineParameters_ = Engine::parse_parameters(GetArguments());

    // Create the Engine, but do not initialize it yet. Subsystems except Graphics & Renderer are registered at this point
    Engine::get_instance();

    // Subscribe to log messages so that can show errors if ErrorExit() is called with empty message
    subscribe_to_event(E_LOGMESSAGE, DV_HANDLER(Application, HandleLogMessage));
}

int Application::Run()
{
#if !defined(__GNUC__) || __EXCEPTIONS
    try
    {
#endif
        Setup();
        if (exitCode_)
            return exitCode_;

        if (!DV_ENGINE.Initialize(engineParameters_))
        {
            ErrorExit();
            return exitCode_;
        }

        Start();
        if (exitCode_)
            return exitCode_;

        while (!DV_ENGINE.IsExiting())
            DV_ENGINE.run_frame();

        Stop();

        SdlHelper::manual_destruct();

        return exitCode_;
#if !defined(__GNUC__) || __EXCEPTIONS
    }
    catch (std::bad_alloc&)
    {
        ErrorDialog(GetTypeName(), "An out-of-memory error occurred. The application will now exit.");
        return EXIT_FAILURE;
    }
#endif
}

void Application::ErrorExit(const String& message)
{
    DV_ENGINE.Exit(); // Close the rendering window
    exitCode_ = EXIT_FAILURE;

    if (!message.Length())
    {
        ErrorDialog(GetTypeName(), startupErrors_.Length() ? startupErrors_ :
            "Application has been terminated due to unexpected error.");
    }
    else
        ErrorDialog(GetTypeName(), message);
}

void Application::HandleLogMessage(StringHash eventType, VariantMap& eventData)
{
    using namespace LogMessage;

    if (eventData[P_LEVEL].GetI32() == LOG_ERROR)
    {
        // Strip the timestamp if necessary
        String error = eventData[P_MESSAGE].GetString();
        i32 bracketPos = error.Find(']');
        if (bracketPos != String::NPOS)
            error = error.Substring(bracketPos + 2);

        startupErrors_ += error + "\n";
    }
}

} // namespace dviglo
