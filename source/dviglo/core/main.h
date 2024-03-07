// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "process_utils.h"

#if defined(_WIN32) && !defined(DV_WIN32_CONSOLE)
#include "mini_dump.h"
#include "../common/win_wrapped.h"
#ifdef _MSC_VER
#include <crtdbg.h>
#endif
#endif

// Define a platform-specific main function, which in turn executes the user-defined function

// MSVC debug mode: use memory leak reporting
#if defined(_MSC_VER) && defined(_DEBUG) && !defined(DV_WIN32_CONSOLE)
#define DV_DEFINE_MAIN(function) \
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) \
{ \
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); \
    dviglo::ParseArguments(GetCommandLineW()); \
    return function; \
}
// MSVC release mode: write minidump on crash
#elif defined(_MSC_VER) && defined(DV_MINIDUMPS) && !defined(DV_WIN32_CONSOLE)
#define DV_DEFINE_MAIN(function) \
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) \
{ \
    dviglo::ParseArguments(GetCommandLineW()); \
    int exitCode; \
    __try \
    { \
        exitCode = function; \
    } \
    __except(dviglo::WriteMiniDump("Urho3D", GetExceptionInformation())) \
    { \
    } \
    return exitCode; \
}
// Other Win32 or minidumps disabled: just execute the function
#elif defined(_WIN32) && !defined(DV_WIN32_CONSOLE)
#define DV_DEFINE_MAIN(function) \
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) \
{ \
    dviglo::ParseArguments(GetCommandLineW()); \
    return function; \
}
// Android or iOS or tvOS: use SDL_main
#elif defined(__ANDROID__) || defined(IOS) || defined(TVOS)
#define DV_DEFINE_MAIN(function) \
extern "C" __attribute__((visibility("default"))) int SDL_main(int argc, char** argv); \
int SDL_main(int argc, char** argv) \
{ \
    dviglo::ParseArguments(argc, argv); \
    return function; \
}
// Linux or OS X: use main
#else
#define DV_DEFINE_MAIN(function) \
int main(int argc, char** argv) \
{ \
    dviglo::ParseArguments(argc, argv); \
    return function; \
}
#endif
