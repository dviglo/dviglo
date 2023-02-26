// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include "../containers/array_ptr.h"
#include "../core/context.h"
#include "../core/core_events.h"
#include "../core/profiler.h"
#include "../core/thread.h"
#include "../engine/engine_events.h"
#include "file.h"
#include "file_system.h"
#include "io_events.h"
#include "log.h"
#include "path.h"

#include <SDL3/SDL_filesystem.h>

#include <sys/stat.h>
#include <cstdio>

#ifdef _WIN32
#ifndef _MSC_VER
#define _WIN32_IE 0x501
#endif
#include "../common/win_wrapped.h"
#include <shellapi.h>
#include <direct.h>
#include <shlobj.h>
#include <sys/types.h>
#include <sys/utime.h>
#else
#include <dirent.h>
#include <cerrno>
#include <unistd.h>
#include <utime.h>
#include <sys/wait.h>
#define MAX_PATH 256
#endif

#include "../common/debug_new.h"

namespace dviglo
{

int DoSystemCommand(const String& commandLine, bool redirectToLog)
{
    if (!redirectToLog)
        return system(commandLine.CString());

    // Get a platform-agnostic temporary file name for stderr redirection
    String stderrFilename;
    String adjustedCommandLine(commandLine);
    char* prefPath = SDL_GetPrefPath("urho3d", "temp");
    if (prefPath)
    {
        stderrFilename = String(prefPath) + "command-stderr";
        adjustedCommandLine += " 2>" + stderrFilename;
        SDL_free(prefPath);
    }

#ifdef _MSC_VER
    #define popen _popen
    #define pclose _pclose
#endif

    // Use popen/pclose to capture the stdout and stderr of the command
    FILE* file = popen(adjustedCommandLine.CString(), "r");
    if (!file)
        return -1;

    // Capture the standard output stream
    char buffer[128];
    while (!feof(file))
    {
        if (fgets(buffer, sizeof(buffer), file))
            DV_LOGRAW(String(buffer));
    }
    int exitCode = pclose(file);

    // Capture the standard error stream
    if (!stderrFilename.Empty())
    {
        SharedPtr<File> errFile(new File(stderrFilename, FILE_READ));
        while (!errFile->IsEof())
        {
            unsigned numRead = errFile->Read(buffer, sizeof(buffer));
            if (numRead)
                Log::WriteRaw(String(buffer, numRead), true);
        }
    }

    return exitCode;
}

int DoSystemRun(const String& fileName, const Vector<String>& arguments)
{
    String fixedFileName = to_native(fileName);

#ifdef _WIN32
    // Add .exe extension if no extension defined
    if (GetExtension(fixedFileName).Empty())
        fixedFileName += ".exe";

    String commandLine = "\"" + fixedFileName + "\"";
    for (unsigned i = 0; i < arguments.Size(); ++i)
        commandLine += " " + arguments[i];

    STARTUPINFOW startupInfo;
    PROCESS_INFORMATION processInfo;
    memset(&startupInfo, 0, sizeof startupInfo);
    memset(&processInfo, 0, sizeof processInfo);

    WString commandLineW(commandLine);
    if (!CreateProcessW(nullptr, (wchar_t*)commandLineW.CString(), nullptr, nullptr, 0, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo))
        return -1;

    WaitForSingleObject(processInfo.hProcess, INFINITE);
    DWORD exitCode;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    return exitCode;
#else
    pid_t pid = fork();
    if (!pid)
    {
        Vector<const char*> argPtrs;
        argPtrs.Push(fixedFileName.CString());
        for (unsigned i = 0; i < arguments.Size(); ++i)
            argPtrs.Push(arguments[i].CString());
        argPtrs.Push(nullptr);

        execvp(argPtrs[0], (char**)&argPtrs[0]);
        return -1; // Return -1 if we could not spawn the process
    }
    else if (pid > 0)
    {
        int exitCode;
        wait(&exitCode);
        return exitCode;
    }
    else
        return -1;
#endif
}

/// Base class for async execution requests.
class AsyncExecRequest : public Thread
{
public:
    /// Construct.
    explicit AsyncExecRequest(unsigned& requestID) :
        requestID_(requestID)
    {
        // Increment ID for next request
        ++requestID;
        if (requestID == M_MAX_UNSIGNED)
            requestID = 1;
    }

    /// Return request ID.
    unsigned GetRequestID() const { return requestID_; }

    /// Return exit code. Valid when IsCompleted() is true.
    int GetExitCode() const { return exitCode_; }

    /// Return completion status.
    bool IsCompleted() const { return completed_; }

protected:
    /// Request ID.
    unsigned requestID_{};
    /// Exit code.
    int exitCode_{};
    /// Completed flag.
    volatile bool completed_{};
};

/// Async system command operation.
class AsyncSystemCommand : public AsyncExecRequest
{
public:
    /// Construct and run.
    AsyncSystemCommand(unsigned requestID, const String& commandLine) :
        AsyncExecRequest(requestID),
        commandLine_(commandLine)
    {
        Run();
    }

    /// The function to run in the thread.
    void ThreadFunction() override
    {
        DV_PROFILE_THREAD("AsyncSystemCommand Thread");

        exitCode_ = DoSystemCommand(commandLine_, false);
        completed_ = true;
    }

private:
    /// Command line.
    String commandLine_;
};

/// Async system run operation.
class AsyncSystemRun : public AsyncExecRequest
{
public:
    /// Construct and run.
    AsyncSystemRun(unsigned requestID, const String& fileName, const Vector<String>& arguments) :
        AsyncExecRequest(requestID),
        fileName_(fileName),
        arguments_(arguments)
    {
        Run();
    }

    /// The function to run in the thread.
    void ThreadFunction() override
    {
        DV_PROFILE_THREAD("AsyncSystemRun Thread");

        exitCode_ = DoSystemRun(fileName_, arguments_);
        completed_ = true;
    }

private:
    /// File to run.
    String fileName_;
    /// Command line split in arguments.
    const Vector<String>& arguments_;
};

FileSystem::FileSystem()
{
    SubscribeToEvent(E_BEGINFRAME, DV_HANDLER(FileSystem, HandleBeginFrame));

    // Subscribe to console commands
    SetExecuteConsoleCommands(true);
}

FileSystem::~FileSystem()
{
    // If any async exec items pending, delete them
    if (asyncExecQueue_.Size())
    {
        for (List<AsyncExecRequest*>::Iterator i = asyncExecQueue_.Begin(); i != asyncExecQueue_.End(); ++i)
            delete(*i);

        asyncExecQueue_.Clear();
    }
}

bool FileSystem::SetCurrentDir(const String& pathName)
{
#ifdef _WIN32
    if (SetCurrentDirectoryW(to_win_native(pathName).CString()) == FALSE)
    {
        DV_LOGERROR("Failed to change directory to " + pathName);
        return false;
    }
#else
    if (chdir(pathName.CString()) != 0)
    {
        DV_LOGERROR("Failed to change directory to " + pathName);
        return false;
    }
#endif

    return true;
}

bool FileSystem::CreateDir(const String& pathName)
{
    // Create each of the parents if necessary
    String parentPath = GetParentPath(pathName);
    if (parentPath.Length() > 1 && !DirExists(parentPath))
    {
        if (!CreateDir(parentPath))
            return false;
    }

#ifdef _WIN32
    bool success = (CreateDirectoryW(to_win_native(trim_end_slash(pathName)).CString(), nullptr) == TRUE) ||
        (GetLastError() == ERROR_ALREADY_EXISTS);
#else
    bool success = mkdir(trim_end_slash(pathName).CString(), S_IRWXU) == 0 || errno == EEXIST;
#endif

    if (success)
        DV_LOGDEBUG("Created directory " + pathName);
    else
        DV_LOGERROR("Failed to create directory " + pathName);

    return success;
}

void FileSystem::SetExecuteConsoleCommands(bool enable)
{
    if (enable == executeConsoleCommands_)
        return;

    executeConsoleCommands_ = enable;
    if (enable)
        SubscribeToEvent(E_CONSOLECOMMAND, DV_HANDLER(FileSystem, HandleConsoleCommand));
    else
        UnsubscribeFromEvent(E_CONSOLECOMMAND);
}

int FileSystem::SystemCommand(const String& commandLine, bool redirectStdOutToLog)
{
    return DoSystemCommand(commandLine, redirectStdOutToLog);
}

int FileSystem::SystemRun(const String& fileName, const Vector<String>& arguments)
{
    return DoSystemRun(fileName, arguments);
}

unsigned FileSystem::SystemCommandAsync(const String& commandLine)
{
#ifdef DV_THREADING
    unsigned requestID = nextAsyncExecID_;
    auto* cmd = new AsyncSystemCommand(nextAsyncExecID_, commandLine);
    asyncExecQueue_.Push(cmd);
    return requestID;
#else
    DV_LOGERROR("Can not execute an asynchronous command as threading is disabled");
    return M_MAX_UNSIGNED;
#endif
}

unsigned FileSystem::SystemRunAsync(const String& fileName, const Vector<String>& arguments)
{
#ifdef DV_THREADING
    unsigned requestID = nextAsyncExecID_;
    auto* cmd = new AsyncSystemRun(nextAsyncExecID_, fileName, arguments);
    asyncExecQueue_.Push(cmd);
    return requestID;
#else
    DV_LOGERROR("Can not run asynchronously as threading is disabled");
    return M_MAX_UNSIGNED;
#endif
}

bool FileSystem::SystemOpen(const String& fileName, const String& mode)
{
    if (!FileExists(fileName) && !DirExists(fileName))
    {
        DV_LOGERROR("File or directory " + fileName + " not found");
        return false;
    }

#ifdef _WIN32
    bool success = (size_t)ShellExecuteW(nullptr, !mode.Empty() ? WString(mode).CString() : nullptr,
        to_win_native(fileName).CString(), nullptr, nullptr, SW_SHOW) > 32;
#else
    Vector<String> arguments;
    arguments.Push(fileName);
    bool success = SystemRun("/usr/bin/xdg-open", arguments) == 0;
#endif
    if (!success)
        DV_LOGERROR("Failed to open " + fileName + " externally");

    return success;
}

bool FileSystem::Copy(const String& srcFileName, const String& destFileName)
{
    SharedPtr<File> srcFile(new File(srcFileName, FILE_READ));
    if (!srcFile->IsOpen())
        return false;
    SharedPtr<File> destFile(new File(destFileName, FILE_WRITE));
    if (!destFile->IsOpen())
        return false;

    unsigned fileSize = srcFile->GetSize();
    SharedArrayPtr<unsigned char> buffer(new unsigned char[fileSize]);

    unsigned bytesRead = srcFile->Read(buffer.Get(), fileSize);
    unsigned bytesWritten = destFile->Write(buffer.Get(), fileSize);
    return bytesRead == fileSize && bytesWritten == fileSize;
}

bool FileSystem::Rename(const String& srcFileName, const String& destFileName)
{
#ifdef _WIN32
    return MoveFileW(to_win_native(srcFileName).CString(), to_win_native(destFileName).CString()) != 0;
#else
    return rename(srcFileName.CString(), destFileName.CString()) == 0;
#endif
}

bool FileSystem::Delete(const String& fileName)
{
#ifdef _WIN32
    return DeleteFileW(to_win_native(fileName).CString()) != 0;
#else
    return remove(fileName.CString()) == 0;
#endif
}

String FileSystem::GetCurrentDir() const
{
#ifdef _WIN32
    wchar_t path[MAX_PATH];
    path[0] = 0;
    GetCurrentDirectoryW(MAX_PATH, path);
    return AddTrailingSlash(String(path));
#else
    char path[MAX_PATH];
    path[0] = 0;
    getcwd(path, MAX_PATH);
    return AddTrailingSlash(String(path));
#endif
}

unsigned FileSystem::GetLastModifiedTime(const String& fileName) const
{
    if (fileName.Empty())
        return 0;

#ifdef _WIN32
    struct _stat st;
    if (!_stat(fileName.CString(), &st))
        return (unsigned)st.st_mtime;
    else
        return 0;
#else
    struct stat st{};
    if (!stat(fileName.CString(), &st))
        return (unsigned)st.st_mtime;
    else
        return 0;
#endif
}

bool FileSystem::FileExists(const String& fileName) const
{
    String fixedName = to_native(trim_end_slash(fileName));

#ifdef _WIN32
    DWORD attributes = GetFileAttributesW(WString(fixedName).CString());
    if (attributes == INVALID_FILE_ATTRIBUTES || attributes & FILE_ATTRIBUTE_DIRECTORY)
        return false;
#else
    struct stat st{};
    if (stat(fixedName.CString(), &st) || st.st_mode & S_IFDIR)
        return false;
#endif

    return true;
}

bool FileSystem::DirExists(const String& pathName) const
{
#ifndef _WIN32
    // Always return true for the root directory
    if (pathName == "/")
        return true;
#endif

    String fixedName = to_native(trim_end_slash(pathName));

#ifdef _WIN32
    DWORD attributes = GetFileAttributesW(WString(fixedName).CString());
    if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY))
        return false;
#else
    struct stat st{};
    if (stat(fixedName.CString(), &st) || !(st.st_mode & S_IFDIR))
        return false;
#endif

    return true;
}

void FileSystem::ScanDir(Vector<String>& result, const String& pathName, const String& filter, unsigned flags, bool recursive) const
{
    result.Clear();

    String initialPath = AddTrailingSlash(pathName);
    ScanDirInternal(result, initialPath, initialPath, filter, flags, recursive);
}

String FileSystem::GetProgramDir() const
{
#if defined(_WIN32)
    wchar_t exeName[MAX_PATH];
    exeName[0] = 0;
    GetModuleFileNameW(nullptr, exeName, MAX_PATH);
    return GetPath(String(exeName));
#elif defined(__linux__)
    char exeName[MAX_PATH];
    memset(exeName, 0, MAX_PATH);
    pid_t pid = getpid();
    String link = "/proc/" + String(pid) + "/exe";
    readlink(link.CString(), exeName, MAX_PATH);
    return GetPath(String(exeName));
#else
    return GetCurrentDir();
#endif
}

String FileSystem::GetUserDocumentsDir() const
{
#if defined(_WIN32)
    wchar_t pathName[MAX_PATH];
    pathName[0] = 0;
    SHGetSpecialFolderPathW(nullptr, pathName, CSIDL_PERSONAL, 0);
    return AddTrailingSlash(String(pathName));
#else
    char pathName[MAX_PATH];
    pathName[0] = 0;
    strcpy(pathName, getenv("HOME"));
    return AddTrailingSlash(String(pathName));
#endif
}

String FileSystem::GetAppPreferencesDir(const String& org, const String& app) const
{
    String dir;
    char* prefPath = SDL_GetPrefPath(org.CString(), app.CString());
    if (prefPath)
    {
        dir = to_internal(String(prefPath));
        SDL_free(prefPath);
    }
    else
        DV_LOGWARNING("Could not get application preferences directory");

    return dir;
}

bool FileSystem::SetLastModifiedTime(const String& fileName, unsigned newTime)
{
    if (fileName.Empty())
        return false;

#ifdef _WIN32
    struct _stat oldTime;
    struct _utimbuf newTimes;
    if (_stat(fileName.CString(), &oldTime) != 0)
        return false;
    newTimes.actime = oldTime.st_atime;
    newTimes.modtime = newTime;
    return _utime(fileName.CString(), &newTimes) == 0;
#else
    struct stat oldTime{};
    struct utimbuf newTimes{};
    if (stat(fileName.CString(), &oldTime) != 0)
        return false;
    newTimes.actime = oldTime.st_atime;
    newTimes.modtime = newTime;
    return utime(fileName.CString(), &newTimes) == 0;
#endif
}

void FileSystem::ScanDirInternal(Vector<String>& result, String path, const String& startPath,
    const String& filter, unsigned flags, bool recursive) const
{
    path = AddTrailingSlash(path);
    String deltaPath;
    if (path.Length() > startPath.Length())
        deltaPath = path.Substring(startPath.Length());

    String filterExtension = filter.Substring(filter.FindLast('.'));
    if (filterExtension.Contains('*'))
        filterExtension.Clear();

#ifdef _WIN32
    WIN32_FIND_DATAW info;
    HANDLE handle = FindFirstFileW(WString(path + "*").CString(), &info);
    if (handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            String fileName(info.cFileName);
            if (!fileName.Empty())
            {
                if (info.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN && !(flags & SCAN_HIDDEN))
                    continue;
                if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (flags & SCAN_DIRS)
                        result.Push(deltaPath + fileName);
                    if (recursive && fileName != "." && fileName != "..")
                        ScanDirInternal(result, path + fileName, startPath, filter, flags, recursive);
                }
                else if (flags & SCAN_FILES)
                {
                    if (filterExtension.Empty() || fileName.EndsWith(filterExtension))
                        result.Push(deltaPath + fileName);
                }
            }
        }
        while (FindNextFileW(handle, &info));

        FindClose(handle);
    }
#else
    DIR* dir;
    struct dirent* de;
    struct stat st{};
    dir = opendir(path.CString());
    if (dir)
    {
        while ((de = readdir(dir)))
        {
            /// \todo Filename may be unnormalized Unicode on Mac OS X. Re-normalize as necessary
            String fileName(de->d_name);
            bool normalEntry = fileName != "." && fileName != "..";
            if (normalEntry && !(flags & SCAN_HIDDEN) && fileName.StartsWith("."))
                continue;
            String pathAndName = path + fileName;
            if (!stat(pathAndName.CString(), &st))
            {
                if (st.st_mode & S_IFDIR)
                {
                    if (flags & SCAN_DIRS)
                        result.Push(deltaPath + fileName);
                    if (recursive && normalEntry)
                        ScanDirInternal(result, path + fileName, startPath, filter, flags, recursive);
                }
                else if (flags & SCAN_FILES)
                {
                    if (filterExtension.Empty() || fileName.EndsWith(filterExtension))
                        result.Push(deltaPath + fileName);
                }
            }
        }
        closedir(dir);
    }
#endif
}

void FileSystem::HandleBeginFrame(StringHash eventType, VariantMap& eventData)
{
    // Go through the execution queue and post + remove completed requests
    for (List<AsyncExecRequest*>::Iterator i = asyncExecQueue_.Begin(); i != asyncExecQueue_.End();)
    {
        AsyncExecRequest* request = *i;
        if (request->IsCompleted())
        {
            using namespace AsyncExecFinished;

            VariantMap& newEventData = GetEventDataMap();
            newEventData[P_REQUESTID] = request->GetRequestID();
            newEventData[P_EXITCODE] = request->GetExitCode();
            SendEvent(E_ASYNCEXECFINISHED, newEventData);

            delete request;
            i = asyncExecQueue_.Erase(i);
        }
        else
            ++i;
    }
}

void FileSystem::HandleConsoleCommand(StringHash eventType, VariantMap& eventData)
{
    using namespace ConsoleCommand;
    if (eventData[P_ID].GetString() == GetTypeName())
        SystemCommand(eventData[P_COMMAND].GetString(), true);
}

void SplitPath(const String& fullPath, String& pathName, String& fileName, String& extension, bool lowercaseExtension)
{
    String fullPathCopy = to_internal(fullPath);

    i32 extPos = fullPathCopy.FindLast('.');
    i32 pathPos = fullPathCopy.FindLast('/');

    if (extPos != String::NPOS && (pathPos == String::NPOS || extPos > pathPos))
    {
        extension = fullPathCopy.Substring(extPos);
        if (lowercaseExtension)
            extension = extension.ToLower();
        fullPathCopy = fullPathCopy.Substring(0, extPos);
    }
    else
        extension.Clear();

    pathPos = fullPathCopy.FindLast('/');
    if (pathPos != String::NPOS)
    {
        fileName = fullPathCopy.Substring(pathPos + 1);
        pathName = fullPathCopy.Substring(0, pathPos + 1);
    }
    else
    {
        fileName = fullPathCopy;
        pathName.Clear();
    }
}

String GetPath(const String& fullPath)
{
    String path, file, extension;
    SplitPath(fullPath, path, file, extension);
    return path;
}

String GetFileName(const String& fullPath)
{
    String path, file, extension;
    SplitPath(fullPath, path, file, extension);
    return file;
}

String GetExtension(const String& fullPath, bool lowercaseExtension)
{
    String path, file, extension;
    SplitPath(fullPath, path, file, extension, lowercaseExtension);
    return extension;
}

String GetFileNameAndExtension(const String& fileName, bool lowercaseExtension)
{
    String path, file, extension;
    SplitPath(fileName, path, file, extension, lowercaseExtension);
    return file + extension;
}

String ReplaceExtension(const String& fullPath, const String& newExtension)
{
    String path, file, extension;
    SplitPath(fullPath, path, file, extension);
    return path + file + newExtension;
}

String AddTrailingSlash(const String& pathName)
{
    String ret = pathName.Trimmed();
    ret.Replace('\\', '/');
    if (!ret.Empty() && ret.Back() != '/')
        ret += '/';
    return ret;
}

String GetParentPath(const String& path)
{
    i32 pos = trim_end_slash(path).FindLast('/');
    if (pos != String::NPOS)
        return path.Substring(0, pos + 1);
    else
        return String();
}

bool IsAbsolutePath(const String& pathName)
{
    if (pathName.Empty())
        return false;

    String path = to_internal(pathName);

    if (path[0] == '/')
        return true;

#ifdef _WIN32
    if (path.Length() > 1 && IsAlpha(path[0]) && path[1] == ':')
        return true;
#endif

    return false;
}

String FileSystem::GetTemporaryDir() const
{
#if defined(_WIN32)
    wchar_t pathName[MAX_PATH];
    pathName[0] = 0;
    GetTempPathW(SDL_arraysize(pathName), pathName);
    return AddTrailingSlash(String(pathName));
#else
    if (char* pathName = getenv("TMPDIR"))
        return AddTrailingSlash(pathName);
#ifdef P_tmpdir
    return AddTrailingSlash(P_tmpdir);
#else
    return "/tmp/";
#endif
#endif
}

}
