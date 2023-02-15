// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../container/str.h"

#include <cstdlib>

namespace dviglo
{

/// Initialize the FPU to round-to-nearest, single precision mode.
DV_API void InitFPU();
/// Display an error dialog with the specified title and message.
DV_API void ErrorDialog(const String& title, const String& message);
/// Exit the application with an error message to the console.
DV_API void ErrorExit(const String& message = String::EMPTY, int exitCode = EXIT_FAILURE);
/// Open a console window.
DV_API void OpenConsoleWindow();
/// Print Unicode text to the console. Will not be printed to the MSVC output window.
DV_API void PrintUnicode(const String& str, bool error = false);
/// Print Unicode text to the console with a newline appended. Will not be printed to the MSVC output window.
DV_API void PrintUnicodeLine(const String& str, bool error = false);
/// Print ASCII text to the console with a newline appended. Uses printf() to allow printing into the MSVC output window.
DV_API void PrintLine(const String& str, bool error = false);
/// Print ASCII text to the console with a newline appended. Uses printf() to allow printing into the MSVC output window.
DV_API void PrintLine(const char* str, bool error = false);
/// Parse arguments from the command line. First argument is by default assumed to be the executable name and is skipped.
DV_API const Vector<String>& ParseArguments(const String& cmdLine, bool skipFirstArgument = true);
/// Parse arguments from the command line.
DV_API const Vector<String>& ParseArguments(const char* cmdLine);
/// Parse arguments from a wide char command line.
DV_API const Vector<String>& ParseArguments(const WString& cmdLine);
/// Parse arguments from a wide char command line.
DV_API const Vector<String>& ParseArguments(const wchar_t* cmdLine);
/// Parse arguments from argc & argv.
DV_API const Vector<String>& ParseArguments(int argc, char** argv);
/// Return previously parsed arguments.
DV_API const Vector<String>& GetArguments();
/// Read input from the console window. Return empty if no input.
DV_API String GetConsoleInput();
/// Return the runtime platform identifier, or (?) if not identified.
DV_API String GetPlatform();
/// Return the number of physical CPU cores.
DV_API unsigned GetNumPhysicalCPUs();
/// Return the number of logical CPUs (different from physical if hyperthreading is used).
DV_API unsigned GetNumLogicalCPUs();
/// Set minidump write location as an absolute path. If empty, uses default (UserProfile/AppData/Roaming/urho3D/crashdumps) Minidumps are only supported on MSVC compiler.
DV_API void SetMiniDumpDir(const String& pathName);
/// Return minidump write location.
DV_API String GetMiniDumpDir();
/// Return the total amount of usable memory in bytes.
DV_API unsigned long long GetTotalMemory();
/// Return the name of the currently logged in user, or (?) if not identified.
DV_API String GetLoginName();
/// Return the name of the running machine.
DV_API String GetHostName();
/// Return the version of the currently running OS, or (?) if not identified.
DV_API String GetOSVersion();

} // namespace dviglo

