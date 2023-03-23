// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../containers/hash_set.h"
#include "../containers/list.h"
#include "../core/object.h"

namespace dviglo
{

class AsyncExecRequest;

/// Return files.
static const unsigned SCAN_FILES = 0x1;
/// Return directories.
static const unsigned SCAN_DIRS = 0x2;
/// Return also hidden files.
static const unsigned SCAN_HIDDEN = 0x4;

/// Subsystem for file and directory operations and access control.
class DV_API FileSystem : public Object
{
    DV_OBJECT(FileSystem, Object);

public:
    static FileSystem& get_instance();

private:
    /// Construct.
    explicit FileSystem();
    /// Destruct.
    ~FileSystem() override;

public:
    // Запрещаем копирование
    FileSystem(const FileSystem&) = delete;
    FileSystem& operator =(const FileSystem&) = delete;

    /// Set the current working directory.
    bool SetCurrentDir(const String& pathName);

    /// Версия функции, которая пишет с лог
    bool create_dir(const String& path);

    /// Set whether to execute engine console commands as OS-specific system command.
    void SetExecuteConsoleCommands(bool enable);
    /// Run a program using the command interpreter, block until it exits and return the exit code
    int SystemCommand(const String& commandLine, bool redirectStdOutToLog = false);
    /// Run a specific program, block until it exits and return the exit code
    int SystemRun(const String& fileName, const Vector<String>& arguments);
    /// Run a program using the command interpreter asynchronously. Return a request ID or M_MAX_UNSIGNED if failed. The exit code will be posted together with the request ID in an AsyncExecFinished event
    unsigned SystemCommandAsync(const String& commandLine);
    /// Run a specific program asynchronously. Return a request ID or M_MAX_UNSIGNED if failed. The exit code will be posted together with the request ID in an AsyncExecFinished event
    unsigned SystemRunAsync(const String& fileName, const Vector<String>& arguments);
    /// Open a file in an external program, with mode such as "edit" optionally specified
    bool SystemOpen(const String& fileName, const String& mode = String::EMPTY);
    /// Copy a file. Return true if successful.
    bool Copy(const String& srcFileName, const String& destFileName);
    /// Rename a file. Return true if successful.
    bool Rename(const String& srcFileName, const String& destFileName);
    /// Delete a file. Return true if successful.
    bool Delete(const String& fileName);
    /// Set a file's last modified time as seconds since 1.1.1970. Return true on success.
    bool SetLastModifiedTime(const String& fileName, unsigned newTime);

    /// Return the absolute current working directory.
    String GetCurrentDir() const;

    /// Return whether is executing engine console commands as OS-specific system command.
    bool GetExecuteConsoleCommands() const { return executeConsoleCommands_; }

    /// Returns the file's last modified time as seconds since 1.1.1970, or 0 if can not be accessed.
    unsigned GetLastModifiedTime(const String& fileName) const;
    /// Check if a file exists.
    bool file_exists(const String& fileName) const;
    /// Scan a directory for specified files.
    void ScanDir(Vector<String>& result, const String& pathName, const String& filter, unsigned flags, bool recursive) const;
    /// Return the program's directory.
    String GetProgramDir() const;
    /// Return the user documents directory.
    String GetUserDocumentsDir() const;
    /// Return path of temporary directory. Path always ends with a forward slash.
    String GetTemporaryDir() const;

private:
    /// Scan directory, called internally.
    void ScanDirInternal
        (Vector<String>& result, String path, const String& startPath, const String& filter, unsigned flags, bool recursive) const;
    /// Handle begin frame event to check for completed async executions.
    void HandleBeginFrame(StringHash eventType, VariantMap& eventData);
    /// Handle a console command event.
    void HandleConsoleCommand(StringHash eventType, VariantMap& eventData);

    /// Async execution queue.
    List<AsyncExecRequest*> asyncExecQueue_;
    /// Next async execution ID.
    unsigned nextAsyncExecID_{1};
    /// Flag for executing engine console commands as OS-specific system command. Default to true.
    bool executeConsoleCommands_{};
};

#define DV_FILE_SYSTEM (dviglo::FileSystem::get_instance())

/// Split a full path to path, filename and extension. The extension will be converted to lowercase by default.
DV_API void
    SplitPath(const String& fullPath, String& pathName, String& fileName, String& extension, bool lowercaseExtension = true);
/// Return the path from a full path.
DV_API String GetPath(const String& fullPath);
/// Return the filename from a full path.
DV_API String GetFileName(const String& fullPath);
/// Return the extension from a full path, converted to lowercase by default.
DV_API String GetExtension(const String& fullPath, bool lowercaseExtension = true);
/// Return the filename and extension from a full path. The case of the extension is preserved by default, so that the file can be opened in case-sensitive operating systems.
DV_API String GetFileNameAndExtension(const String& fileName, bool lowercaseExtension = false);
/// Replace the extension of a file name with another.
DV_API String ReplaceExtension(const String& fullPath, const String& newExtension);
/// Add a slash at the end of the path if missing and convert to internal format (use slashes).
DV_API String AddTrailingSlash(const String& pathName);
/// Return whether a path is absolute.
DV_API bool IsAbsolutePath(const String& pathName);

}
