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
#include "fs_base.h"
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
        return system(commandLine.c_str());

    // Get a platform-agnostic temporary file name for stderr redirection
    String stderrFilename;
    String adjustedCommandLine(commandLine);
    String pref_path = get_pref_path("urho3d", "temp");

    if (!pref_path.Empty())
    {
        stderrFilename = pref_path + "command-stderr";
        adjustedCommandLine += " 2>" + stderrFilename;
    }

#ifdef _MSC_VER
    #define popen _popen
    #define pclose _pclose
#endif

    // Use popen/pclose to capture the stdout and stderr of the command
    FILE* file = popen(adjustedCommandLine.c_str(), "r");
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
    if (!CreateProcessW(nullptr, (wchar_t*)commandLineW.c_str(), nullptr, nullptr, 0, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo))
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
        argPtrs.Push(fixedFileName.c_str());
        for (unsigned i = 0; i < arguments.Size(); ++i)
            argPtrs.Push(arguments[i].c_str());
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

#ifdef _DEBUG
// Проверяем, что не происходит обращения к синглтону после вызова деструктора
static bool file_system_destructed = false;
#endif

// Определение должно быть в cpp-файле, иначе будут проблемы в shared-версии движка в MinGW.
// Когда функция в h-файле, в exe и в dll создаются свои экземпляры объекта с разными адресами.
// https://stackoverflow.com/questions/71830151/why-singleton-in-headers-not-work-for-windows-mingw
FileSystem& FileSystem::get_instance()
{
    assert(!file_system_destructed);
    static FileSystem instance;
    return instance;
}

FileSystem::FileSystem()
{
    SubscribeToEvent(E_BEGINFRAME, DV_HANDLER(FileSystem, HandleBeginFrame));

    // Subscribe to console commands
    SetExecuteConsoleCommands(true);

    DV_LOGDEBUG("Singleton FileSystem constructed");
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

    DV_LOGDEBUG("Singleton FileSystem destructed");

#ifdef _DEBUG
    file_system_destructed = true;
#endif
}

bool FileSystem::SetCurrentDir(const String& pathName)
{
#ifdef _WIN32
    if (SetCurrentDirectoryW(to_win_native(pathName).c_str()) == FALSE)
    {
        DV_LOGERROR("Failed to change directory to " + pathName);
        return false;
    }
#else
    if (chdir(pathName.c_str()) != 0)
    {
        DV_LOGERROR("Failed to change directory to " + pathName);
        return false;
    }
#endif

    return true;
}

bool FileSystem::create_dir(const String& path)
{
    bool ret = create_dir_silent(path);

    if (ret)
        DV_LOGDEBUG("Created directory " + path);
    else
        DV_LOGERROR("Failed to create directory " + path);

    return ret;
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

int FileSystem::system_command(const String& commandLine, bool redirectStdOutToLog)
{
    return DoSystemCommand(commandLine, redirectStdOutToLog);
}

int FileSystem::SystemRun(const String& fileName, const Vector<String>& arguments)
{
    return DoSystemRun(fileName, arguments);
}

unsigned FileSystem::system_command_async(const String& commandLine)
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

bool FileSystem::system_open(const String& fileName, const String& mode)
{
    if (!file_exists(fileName) && !dir_exists(fileName))
    {
        DV_LOGERROR("File or directory " + fileName + " not found");
        return false;
    }

#ifdef _WIN32
    bool success = (size_t)ShellExecuteW(nullptr, !mode.Empty() ? WString(mode).c_str() : nullptr,
        to_win_native(fileName).c_str(), nullptr, nullptr, SW_SHOW) > 32;
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
    return MoveFileW(to_win_native(srcFileName).c_str(), to_win_native(destFileName).c_str()) != 0;
#else
    return rename(srcFileName.c_str(), destFileName.c_str()) == 0;
#endif
}

bool FileSystem::Delete(const String& fileName)
{
#ifdef _WIN32
    return DeleteFileW(to_win_native(fileName).c_str()) != 0;
#else
    return remove(fileName.c_str()) == 0;
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
    if (!_stat(fileName.c_str(), &st))
        return (unsigned)st.st_mtime;
    else
        return 0;
#else
    struct stat st{};
    if (!stat(fileName.c_str(), &st))
        return (unsigned)st.st_mtime;
    else
        return 0;
#endif
}

bool FileSystem::file_exists(const String& fileName) const
{
    String fixedName = to_native(trim_end_slash(fileName));

#ifdef _WIN32
    DWORD attributes = GetFileAttributesW(WString(fixedName).c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || attributes & FILE_ATTRIBUTE_DIRECTORY)
        return false;
#else
    struct stat st{};
    if (stat(fixedName.c_str(), &st) || st.st_mode & S_IFDIR)
        return false;
#endif

    return true;
}

void FileSystem::scan_dir(Vector<String>& result, const String& pathName, const String& filter, unsigned flags, bool recursive) const
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
    readlink(link.c_str(), exeName, MAX_PATH);
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

bool FileSystem::SetLastModifiedTime(const String& fileName, unsigned newTime)
{
    if (fileName.Empty())
        return false;

#ifdef _WIN32
    struct _stat oldTime;
    struct _utimbuf newTimes;
    if (_stat(fileName.c_str(), &oldTime) != 0)
        return false;
    newTimes.actime = oldTime.st_atime;
    newTimes.modtime = newTime;
    return _utime(fileName.c_str(), &newTimes) == 0;
#else
    struct stat oldTime{};
    struct utimbuf newTimes{};
    if (stat(fileName.c_str(), &oldTime) != 0)
        return false;
    newTimes.actime = oldTime.st_atime;
    newTimes.modtime = newTime;
    return utime(fileName.c_str(), &newTimes) == 0;
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
    HANDLE handle = FindFirstFileW(WString(path + "*").c_str(), &info);
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
    dir = opendir(path.c_str());
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
            if (!stat(pathAndName.c_str(), &st))
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
        system_command(eventData[P_COMMAND].GetString(), true);
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
